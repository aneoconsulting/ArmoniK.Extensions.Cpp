#pragma once

#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <condition_variable>
#include <mutex>
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
private:
  /**
   * @brief The maximum number of threads in the pool
   */
  std::size_t max_threads_;

  /**
   * @brief The number of sleeping threads
   */
  std::size_t sleeping_threads_;

  /**
   * @brief Mutex to protect the pool
   */
  std::mutex mutex_;

  /**
   * @brief Condition variable to notify threads of new tasks
   */
  std::condition_variable condition_;

  /**
   * @brief Logger
   */
  armonik::api::common::logger::Logger *logger_;

  /**
   * @brief The threads in the pool
   */
  std::vector<std::thread> threads_;

  /**
   * @brief The task queue
   */
  std::vector<std::function<void()>> queue_;

  /**
   * @brief Flag to stop the pool
   */
  bool stop_;

private:
  /**
   * @brief The main loop for each thread
   */
  void Run();

public:
  /**
   * @brief Creates a thread pool
   * @param max_threads Maximum number of threads
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
   * @brief Move constructor
   *
   * @param other Other thread pool
   */
  ThreadPool(ThreadPool &&other) noexcept;
  /**
   * @brief Move assignment constructor
   *
   * @param other Other thread pool
   */
  ThreadPool &operator=(ThreadPool &&other) noexcept;

  /**
   * @brief Destroy the thread Pool object, waiting for all threads to finish
   *
   */
  ~ThreadPool();

  /**
   * @brief Spawn a task ont the pool
   *
   * @param f The task to execute
   */
  void Spawn(std::function<void()> f);
};

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
