#include "ThreadPool.h"

#include <sstream>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

  /**
   * @brief A task to execute
   */
class ThreadPool::Task {
private:
  /**
   * @brief The function to execute
   */
  std::function<void()> func_;

  /**
   * @brief The join set this task belongs to, optional
   */
  ThreadPool::JoinSet *join_set_ = nullptr;

private:
  friend class ThreadPool;

public:
/**
 * @brief Default constructor
 */
  Task() = default;

  /**
   * @brief Creates a task with the given function and optional join set
   */
  Task(std::function<void()> func, ThreadPool::JoinSet *join_set = nullptr)
      : func_(std::move(func)), join_set_(join_set) {
    if (join_set_) {
      // Increment the task count in the join set
      std::unique_lock<std::mutex> lock(join_set_->thread_pool_.mutex_);
      join_set_->task_count_ += 1;
    }
  }

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
  Task(Task &&other) noexcept : func_(std::move(other.func_)), join_set_(other.join_set_) { other.join_set_ = nullptr; }

  /**
   * @brief Move assignment operator
   */
  Task &operator=(Task &&other) noexcept {
    if (this != &other) {
      func_ = std::move(other.func_);
      join_set_ = other.join_set_;
      other.join_set_ = nullptr;
    }
    return *this;
  }

  /**
   * @brief Destroy the Task object, updating the join set if applicable
   */
  ~Task() {
    if (join_set_) {
      bool notify = false;
      { // Decrement the task count in the join set
        std::unique_lock<std::mutex> lock(join_set_->mutex_);
        join_set_->task_count_ -= 1;
        notify = (join_set_->task_count_ == 0);
      }
      if (notify) {
        join_set_->wake_condition_.notify_all();
      }
    }
  }

  /**
   * @brief Execute the task
   */
  void Execute() { func_(); }
};

ThreadPool::ThreadPool(int max_threads, armonik::api::common::logger::Logger &logger)
    : max_threads_(max_threads == 0 ? std::thread::hardware_concurrency() : max_threads), sleeping_threads_(0),
      logger_(logger), stop_(false) {
  Logger().debug("ThreadPool created", {{"max_threads", std::to_string(max_threads_)}});
}

ThreadPool::~ThreadPool() {
  auto logger = Logger();
  logger.verbose("ThreadPool is stopping...");
  { // Notify all threads to stop
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
  }
  logger.verbose("Notifying all threads to stop...");
  condition_.notify_all();

  // Wait for all threads to finish
  for (std::thread &thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  logger.debug("ThreadPool stopped");
}

armonik::api::common::logger::LocalLogger ThreadPool::Logger(armonik::api::common::logger::Context context) {
  context.emplace("thread_pool_id", std::to_string(reinterpret_cast<std::uintptr_t>(this)));
  return logger_.local(std::move(context));
}

void ThreadPool::Run() {
  std::stringstream ss;
  ss << std::this_thread::get_id();
  armonik::api::common::logger::Context context{{"thread_id", ss.str()}};
  auto logger = Logger(context);

  logger.debug("Thread started");

  while (true) {
    Task task;

    { // Lock the pool to get a new task
      std::unique_lock<std::mutex> lock(mutex_);

      // Wait for a task or stop signal
      ++sleeping_threads_;
      condition_.wait(lock, [this]() { return stop_ || !pending_tasks_.empty(); });
      --sleeping_threads_;

      // If the stopping of the pool has been requested and there is no more task, exit the thread
      if (stop_ && pending_tasks_.empty()) {
        return;
      }

      // Get the next task
      task = std::move(pending_tasks_.back());
      pending_tasks_.pop_back();
    }

    auto task_logger = task.join_set_ ? task.join_set_->Logger(context) : Logger(context);
    task_logger.verbose("Got a new task to execute");

    // Execute the task
    try {
      task.Execute();
    } catch (const std::exception &e) {
      task_logger.error("Exception in thread pool task: " + std::string(e.what()));
    } catch (...) {
      task_logger.error("Unknown exception in thread pool task");
    }

    // Task destructor will handle JoinSet bookkeeping
  }

  logger.debug("Thread stopped");
}

void ThreadPool::Spawn(Task task) {
  auto logger = task.join_set_ ? task.join_set_->Logger() : Logger();
  logger.verbose("Spawning new task");

  { // Lock the pool to enqueue a new task
    std::unique_lock<std::mutex> lock(mutex_);
    if (stop_) {
      throw std::runtime_error("Spawn on stopped ThreadPool");
    }

    // Enqueue the task
    pending_tasks_.emplace_back(std::move(task));

    // If there are no sleeping threads and we have not reached max threads, create a new thread
    if (sleeping_threads_ == 0 && threads_.size() < max_threads_) {
      threads_.emplace_back([this]() { Run(); });
    }
  }

  // Notify one thread that there is a new task available
  condition_.notify_one();
}

void ThreadPool::Spawn(std::function<void()> f) { Spawn(Task(std::move(f))); }

ThreadPool::JoinSet::JoinSet(ThreadPool &thread_pool) : thread_pool_(thread_pool), task_count_(0) {
  Logger().debug("JoinSet created");
}

ThreadPool::JoinSet::~JoinSet() {
  std::unique_lock<std::mutex> lock(mutex_);
  wake_condition_.wait(lock, [this]() { return task_count_ == 0; });

  Logger().debug("JoinSet destroyed");
}

armonik::api::common::logger::LocalLogger ThreadPool::JoinSet::Logger(armonik::api::common::logger::Context context) {
  context.emplace("join_set_id", std::to_string(reinterpret_cast<std::uintptr_t>(this)));
  return thread_pool_.Logger(std::move(context));
}

void ThreadPool::JoinSet::Spawn(std::function<void()> f) { thread_pool_.Spawn(Task(std::move(f), this)); }

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
