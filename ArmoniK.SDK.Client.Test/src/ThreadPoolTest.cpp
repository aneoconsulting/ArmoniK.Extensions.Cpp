#include <gtest/gtest.h>

#include "ThreadPool.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

using namespace ArmoniK::Sdk::Client::Internal;

constexpr auto TIMEOUT = std::chrono::seconds(1);

// clang-format off
#define DELETE_OR_ABORT(ptr, timeout)     \
  do {                                    \
    if (!DeleteOrAbort(ptr, timeout)) {   \
      return;                             \
    }                                     \
  } while (0)
// clang-format on

/**
 * @brief Try to delete a pointer in a separate thread, aborting the test if it blocks for too long
 * @tparam T The type of pointer to delete
 * @param ptr The pointer to delete
 */
template <typename T> bool DeleteOrAbort(T *ptr, std::chrono::milliseconds timeout) {
  auto promise = std::make_shared<std::promise<void>>();

  std::thread delete_thread([=]() {
    delete ptr;
    promise->set_value();
  });

  bool success = promise->get_future().wait_for(timeout) == std::future_status::ready;

  EXPECT_TRUE(success) << "Destructor blocked for more than " << timeout.count() << " milliseconds, leaking pointer";

  if (success) {
    delete_thread.join();
  } else {
    delete_thread.detach();
  }

  return success;
}

class ThreadPoolTest : public ::testing::Test {
protected:
  std::unique_ptr<armonik::api::common::logger::Logger> logger_;

  void SetUp() override {
    // Initialize logger for tests
    logger_ = std::make_unique<armonik::api::common::logger::Logger>(
        armonik::api::common::logger::writer_console(), armonik::api::common::logger::formatter_plain(true),
        armonik::api::common::logger::Level::Verbose);
  }
};

TEST_F(ThreadPoolTest, ConstructorCreatesThreadPool) {
  ASSERT_NO_THROW({ ThreadPool pool(4, *logger_); });
}

TEST_F(ThreadPoolTest, SpawnSingleTask) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  std::promise<void> task_promise;
  auto task_future = task_promise.get_future();

  pool->Spawn([&task_promise]() { task_promise.set_value(); });

  ASSERT_EQ(task_future.wait_for(TIMEOUT), std::future_status::ready);
  DELETE_OR_ABORT(pool, TIMEOUT);
}

TEST_F(ThreadPoolTest, SpawnMultipleTasks) {
  ThreadPool *pool = new ThreadPool(4, *logger_);
  std::vector<std::future<void>> futures;

  for (int i = 0; i < 10; ++i) {
    std::promise<void> promise;
    futures.push_back(promise.get_future());
    pool->Spawn([promise = std::move(promise)]() mutable { promise.set_value(); });
  }

  for (size_t i = 0; i < futures.size(); ++i) {
    ASSERT_EQ(futures[i].wait_for(TIMEOUT), std::future_status::ready)
        << "Future " << i << " did not complete within timeout";
  }
  DELETE_OR_ABORT(pool, TIMEOUT);
}

TEST_F(ThreadPoolTest, JoinSetCreation) {
  ASSERT_NO_THROW({
    ThreadPool pool(2, *logger_);
    ThreadPool::JoinSet join_set(pool);
  });
}

TEST_F(ThreadPoolTest, JoinSetSpawnSingleTask) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  ThreadPool::JoinSet *join_set = new ThreadPool::JoinSet(*pool);
  std::promise<void> task_promise;
  auto task_future = task_promise.get_future();

  join_set->Spawn([&task_promise]() { task_promise.set_value(); });

  ASSERT_EQ(task_future.wait_for(TIMEOUT), std::future_status::ready);
  DELETE_OR_ABORT(join_set, TIMEOUT);
  DELETE_OR_ABORT(pool, TIMEOUT);
}

TEST_F(ThreadPoolTest, JoinSetSpawnMultipleTasks) {
  ThreadPool *pool = new ThreadPool(4, *logger_);
  ThreadPool::JoinSet *join_set = new ThreadPool::JoinSet(*pool);
  std::vector<std::future<void>> futures;

  for (int i = 0; i < 20; ++i) {
    std::promise<void> promise;
    futures.push_back(promise.get_future());
    join_set->Spawn([promise = std::move(promise)]() mutable { promise.set_value(); });
  }

  for (size_t i = 0; i < futures.size(); ++i) {
    ASSERT_EQ(futures[i].wait_for(TIMEOUT), std::future_status::ready)
        << "Future " << i << " did not complete within timeout";
  }
  DELETE_OR_ABORT(join_set, TIMEOUT);
  DELETE_OR_ABORT(pool, TIMEOUT);
}

TEST_F(ThreadPoolTest, JoinSetWaitsForCompletion) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  std::vector<std::future<void>> futures;

  ThreadPool::JoinSet *join_set = new ThreadPool::JoinSet(*pool);
  for (int i = 0; i < 5; ++i) {
    std::promise<void> promise;
    futures.push_back(promise.get_future());
    join_set->Spawn([promise = std::move(promise)]() mutable { promise.set_value(); });
  }
  DELETE_OR_ABORT(join_set, TIMEOUT);

  for (size_t i = 0; i < futures.size(); ++i) {
    ASSERT_EQ(futures[i].wait_for(TIMEOUT), std::future_status::ready)
        << "Future " << i << " did not complete within timeout";
  }
  DELETE_OR_ABORT(pool, TIMEOUT);
}

TEST_F(ThreadPoolTest, JoinSetDestructorWaitsForCompletion) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  std::vector<std::future<void>> futures;

  ThreadPool::JoinSet *join_set = new ThreadPool::JoinSet(*pool);
  for (int i = 0; i < 5; ++i) {
    std::promise<void> promise;
    futures.push_back(promise.get_future());
    join_set->Spawn([promise = std::move(promise)]() mutable {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      promise.set_value();
    });
  }
  DELETE_OR_ABORT(join_set, TIMEOUT);

  for (size_t i = 0; i < futures.size(); ++i) {
    ASSERT_EQ(futures[i].wait_for(TIMEOUT), std::future_status::ready)
        << "Future " << i << " did not complete within timeout";
  }
  DELETE_OR_ABORT(pool, TIMEOUT);
}

TEST_F(ThreadPoolTest, ThreadPoolDestructorWaitsForCompletion) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  std::vector<std::future<void>> futures;

  for (int i = 0; i < 8; ++i) {
    std::promise<void> promise;
    futures.push_back(promise.get_future());
    pool->Spawn([promise = std::move(promise)]() mutable { promise.set_value(); });
  }

  for (size_t i = 0; i < futures.size(); ++i) {
    ASSERT_EQ(futures[i].wait_for(TIMEOUT), std::future_status::ready)
        << "Future " << i << " did not complete within timeout";
  }
  DELETE_OR_ABORT(pool, TIMEOUT);
}

TEST_F(ThreadPoolTest, ConcurrentTaskExecution) {
  ThreadPool *pool = new ThreadPool(4, *logger_);
  std::atomic<int> concurrent_count(0);
  std::atomic<int> max_concurrent(0);
  std::vector<std::future<void>> futures;

  for (int i = 0; i < 8; ++i) {
    std::promise<void> promise;
    futures.push_back(promise.get_future());
    pool->Spawn([&, promise = std::move(promise)]() mutable {
      ++concurrent_count;
      {
        int current = concurrent_count.load();
        int expected = max_concurrent.load();
        while (current > expected && !max_concurrent.compare_exchange_weak(expected, current)) {
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      --concurrent_count;
      promise.set_value();
    });
  }

  for (size_t i = 0; i < futures.size(); ++i) {
    ASSERT_EQ(futures[i].wait_for(TIMEOUT), std::future_status::ready)
        << "Future " << i << " did not complete within timeout";
  }
  ASSERT_GT(max_concurrent, 1);
  DELETE_OR_ABORT(pool, TIMEOUT);
}
