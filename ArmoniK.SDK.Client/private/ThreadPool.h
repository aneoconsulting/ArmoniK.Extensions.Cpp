#pragma once

#include "Function.h"
#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {
/**
 * @brief A thread pool to execute tasks in background
 */
class ThreadPool {
public:
  /**
   * @brief A join set to wait for a set of tasks to finish
   */
  class JoinSet;

private:
  /**
   * @brief A task to execute
   */
  class Task {
  private:
    /**
     * @brief The function to execute
     */
    Function<void()> func_;

    /**
     * @brief The join set this task belongs to, optional
     */
    JoinSet *join_set_ = nullptr;

  private:
    friend class ThreadPool;

  public:
    /**
     * @brief Default constructor
     */
    Task();

    /**
     * @brief Creates a task with the given function and optional join set
     */
    Task(Function<void()> &&func, JoinSet *join_set = nullptr);

    /**
     * @brief Copy constructor
     */
    Task(const Task &) = delete;

    /**
     * @brief Copy assignment operator
     */
    Task &operator=(const Task &) = delete;

    /**
     * @brief Move constructor
     */
    Task(Task &&other) noexcept;

    /**
     * @brief Move assignment operator
     */
    Task &operator=(Task &&other) noexcept;

    /**
     * @brief Destroy the Task object, updating the join set if applicable
     */
    ~Task();

    /**
     * @brief Execute the task
     */
    void Execute(armonik::api::common::logger::ILogger &logger);

    /**
     * @brief Record current error for the join set
     */
    void RecordError();
  };

private:
  /**
   * @brief Scheduling state of a pool thread
   *
   * Invariants, with L = max_threads_ and C = max_blocked_: #Running + #Ready <= L and
   * #Blocked + #Pending <= C, except for scopes that do not enforce C (see BlockingScope). Reserved,
   * Blocked and Pending threads are not counted against L, so the pool may own about L + C threads.
   */
  enum class WorkerState {
    Running,  ///< Executing a task, or about to pick one from the queue
    Ready,    ///< Idle, holding one of the L slots
    Reserved, ///< Idle, holding no slot; woken up before creating a new thread
    Blocked,  ///< Inside a BlockingScope, holding no slot
    Pending,  ///< Left a BlockingScope, waiting for a slot to resume its task
  };

  /**
   * @brief A thread owned by the pool
   */
  struct Worker {
    WorkerState state = WorkerState::Running;
    std::condition_variable wake;
    std::thread thread;
  };

  /**
   * @brief The maximum number of threads that may be Running or Ready at once
   */
  std::size_t max_threads_;

  /**
   * @brief The maximum number of threads that may be Blocked or Pending at once
   */
  std::size_t max_blocked_;

  /**
   * @brief Number of Running threads
   */
  std::size_t running_;

  /**
   * @brief Number of Blocked or Pending threads
   */
  std::size_t blocked_;

  /**
   * @brief Mutex to protect the pool
   */
  std::mutex mutex_;

  /**
   * @brief Logger
   */
  armonik::api::common::logger::Logger &logger_;

  /**
   * @brief All the threads ever created by the pool, joined on destruction
   */
  std::vector<std::unique_ptr<Worker>> workers_;

  /**
   * @brief Ready threads
   */
  std::vector<Worker *> ready_;

  /**
   * @brief Reserved threads
   */
  std::vector<Worker *> reserved_;

  /**
   * @brief Pending threads, resumed in FIFO order
   */
  std::deque<Worker *> pending_;

  /**
   * @brief The tasks waiting for a Running thread
   */
  std::queue<Task> pending_tasks_;

  /**
   * @brief Flag to stop the pool
   */
  bool stop_;

  /**
   * @brief The pool owning the calling thread, if any
   */
  static thread_local ThreadPool *current_pool_;

  /**
   * @brief The calling thread, if owned by a pool and not inside a BlockingScope
   */
  static thread_local Worker *current_worker_;

private:
  /**
   * @brief Create a local logger for the thread pool
   */
  armonik::api::common::logger::LocalLogger Logger(armonik::api::common::logger::Context context = {});

  /**
   * @brief The main loop for each thread
   */
  void Run(Worker &worker);

  /**
   * @brief Spawn a task on the pool
   *
   * @param f The task to execute
   */
  void Spawn(Task &&);

  /**
   * @brief Make a Reserved thread (or a new one) Running, for a queued task. Requires mutex_.
   * @throw std::system_error if a new thread cannot be created, leaving the pool unchanged
   */
  void StartThread();

  /**
   * @brief Make the calling Running thread wait as Ready or Reserved until it is Running again.
   * Requires mutex_ held through lock.
   * @return false if the pool is stopping and the thread must exit
   */
  bool Idle(Worker &worker, std::unique_lock<std::mutex> &lock);

  /**
   * @brief Running -> Blocked, handing the released slot to a Pending thread or to a queued task
   * @return false, leaving the thread Running, if enforce_limit and max_blocked_ threads are already
   * Blocked or Pending
   */
  bool Block(Worker &worker, bool enforce_limit);

  /**
   * @brief Blocked -> Running if a slot is free, Blocked -> Pending (waiting for one) otherwise
   */
  void Unblock(Worker &worker);

public:
  /**
   * @brief RAII marker for a section where the calling thread waits on a condition that other pool
   * tasks may have to satisfy (JoinSet::Wait(), a ByteBudget reservation...).
   *
   * On a pool thread, the thread leaves its slot for the duration of the section, so the tasks it
   * waits for can still run. Leaving the section may wait for a slot to free up. Off a pool thread,
   * or nested inside another BlockingScope, it does nothing.
   *
   * If the pool already has its maximum number of threads inside a BlockingScope, the constructor
   * throws std::runtime_error. With enforce_limit = false (e.g. in a destructor, which must not
   * throw), it goes past the limit instead: keeping the slot could deadlock the awaited tasks.
   *
   * @warning Do not hold a lock while leaving the section: it may wait for other pool tasks.
   */
  class BlockingScope {
  public:
    explicit BlockingScope(bool enforce_limit = true);
    ~BlockingScope();
    BlockingScope(const BlockingScope &) = delete;
    BlockingScope &operator=(const BlockingScope &) = delete;

  private:
    ThreadPool *pool_;
    Worker *worker_;
  };

  /**
   * @brief Creates a thread pool
   * @param max_threads Maximum number of threads running tasks at once, 0 for hardware concurrency
   * @param logger Logger
   * @param max_blocked_threads Maximum number of threads inside a BlockingScope at once, 0 for max_threads
   * @note SessionServiceImpl relies on the default: it keeps at most max_threads result handlers in
   * flight (see SessionServiceImpl::handler_budget_), and only handlers block in pool tasks, each in at
   * most one BlockingScope at a time. Keep both limits in sync.
   */
  explicit ThreadPool(int max_threads, armonik::api::common::logger::Logger &logger, int max_blocked_threads = 0);

  /**
   * @brief Copy constructor
   *
   * @param other Other thread pool
   */
  ThreadPool(const ThreadPool &) = delete;

  /**
   * @brief Copy operator
   *
   * @param other Other thread pool
   */
  ThreadPool &operator=(const ThreadPool &) = delete;

  /**
   * @brief Destroy the thread Pool object, waiting for all threads to finish
   *
   */
  ~ThreadPool();

  /**
   * @brief Spawn a task on the pool
   *
   * @param f The task to execute
   */
  void Spawn(Function<void()> &&f);

  /**
   * @brief The maximum number of threads running tasks at once
   */
  [[nodiscard]] std::size_t MaxThreads() const { return max_threads_; }

  /**
   * @brief Whether the calling thread is owned by a thread pool
   */
  static bool IsWorkerThread() { return current_pool_ != nullptr; }

  /**
   * @brief Wait on cv until pred() holds, inside a BlockingScope if it does not hold right away
   *
   * pred() is always evaluated with m locked, and may update the state m protects when it returns
   * true (e.g. to reserve a resource atomically with the check). Returns with m unlocked.
   * enforce_limit is forwarded to BlockingScope.
   */
  template <typename Pred>
  static void BlockingWait(std::mutex &m, std::condition_variable &cv, Pred pred, bool enforce_limit = true) {
    {
      std::lock_guard<std::mutex> lock(m);
      if (pred()) {
        return;
      }
    }
    BlockingScope blocking(enforce_limit);
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, pred);
  }
};

/**
 * @brief A join set to wait for a set of tasks to finish
 */
class ThreadPool::JoinSet {
private:
  friend class ThreadPool;

private:
  /**
   * @brief The thread pool on which tasks are spawned
   */
  ThreadPool &thread_pool_;

  /**
   * @brief The number of unfinished tasks in the join set
   */
  std::size_t task_count_;

  /**
   * @brief The exception that occurred in any task of the join set, if any
   */
  std::exception_ptr exception_;

  /**
   * @brief Mutex to protect the join set
   */
  std::mutex mutex_;

  /**
   * @brief Condition variable to wait for tasks to finish
   */
  std::condition_variable wake_condition_;

private:
  /**
   * @brief Create a local logger for the join set
   */
  armonik::api::common::logger::LocalLogger Logger(armonik::api::common::logger::Context context = {});

public:
  /**
   * @brief Creates a join set on the given thread pool
   *
   * @param thread_pool The thread pool
   */
  explicit JoinSet(ThreadPool &);

  /**
   * @brief Copy constructor
   */
  JoinSet(const JoinSet &) = delete;

  /**
   * @brief Copy assignment operator
   */
  JoinSet &operator=(const JoinSet &) = delete;

  /**
   * @brief Destroy the Join Set object, waiting for all tasks to finish
   * @note Ignore all exceptions that could have been thrown by the tasks
   */
  ~JoinSet();

  /**
   * @brief Spawn a task on the associated thread pool and add it to the join set
   *
   * @param f The task to execute
   */
  void Spawn(Function<void()> &&f);

  /**
   * @brief Wait for all tasks in the join set to finish
   * @throw If any task has thrown, the exception is thrown by Wait()
   * @note In case of an exception, it does not block until all tasks have finished
   */
  void Wait();
};

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
