#include "ThreadPool.h"

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

ThreadPool::ThreadPool(int max_threads, armonik::api::common::logger::Logger &logger)
    : max_threads_(max_threads == 0 ? std::thread::hardware_concurrency() : max_threads), sleeping_threads_(0),
      logger_(&logger), stop_(false) {}

ThreadPool::ThreadPool(ThreadPool &&other) noexcept = default;
ThreadPool &ThreadPool::operator=(ThreadPool &&other) noexcept = default;

ThreadPool::~ThreadPool() {
  { // Notify all threads to stop
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
  }
  condition_.notify_all();

  // Wait for all threads to finish
  for (std::thread &thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void ThreadPool::Run() {
  while (true) {
    std::function<void()> task;

    { // Lock the pool to get a new task
      std::unique_lock<std::mutex> lock(mutex_);

      // Wait for a task or stop signal
      ++sleeping_threads_;
      condition_.wait(lock, [this]() { return stop_ || !queue_.empty(); });
      --sleeping_threads_;

      // If the stopping of the pool has been requested and there is no more task, exit the thread
      if (stop_ && queue_.empty()) {
        return;
      }

      // Get the next task
      task = std::move(queue_.back());
      queue_.pop_back();
    }

    // Execute the task
    task();
  }
}

void ThreadPool::Spawn(std::function<void()> f) {
  { // Lock the pool to enqueue a new task
    std::unique_lock<std::mutex> lock(mutex_);
    if (stop_) {
      throw std::runtime_error("Spawn on stopped ThreadPool");
    }

    // Enqueue the task
    queue_.emplace_back(std::move(f));

    // If there are no sleeping threads and we have not reached max threads, create a new thread
    if (sleeping_threads_ == 0 && threads_.size() < max_threads_) {
      threads_.emplace_back([this]() { Run(); });
    }
  }

  // Notify one thread that there is a new task available
  condition_.notify_one();
}

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
