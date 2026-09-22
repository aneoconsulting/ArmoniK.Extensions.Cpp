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
   * Invariant, with L = max_threads_: #Running + #Ready <= L. Reserved, Blocked and Pending threads
   * are not counted against L, so the pool may own more than L threads.
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
   * @brief Number of Running threads
   */
  std::size_t running_;

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
   */
  void Block(Worker &worker);

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
   * @warning Do not hold a lock while leaving the section: it may wait for other pool tasks.
   */
  class BlockingScope {
  public:
    BlockingScope();
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
   */
  explicit ThreadPool(int max_threads, armonik::api::common::logger::Logger &logger);

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
   * @brief Wait on cv until pred() holds, inside a BlockingScope if it does not hold right away
   *
   * pred() is always evaluated with m locked, and may update the state m protects when it returns
   * true (e.g. to reserve a resource atomically with the check). Returns with m unlocked.
   */
  template <typename Pred> static void BlockingWait(std::mutex &m, std::condition_variable &cv, Pred pred) {
    {
      std::lock_guard<std::mutex> lock(m);
      if (pred()) {
        return;
      }
    }
    BlockingScope blocking;
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
