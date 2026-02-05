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
/**
 * @brief Try to execute a statement in a separate thread, aborting the test if it blocks for too long
 * @param timeout The time to wait before aborting
 * @param ... The statement to execute
 */
#define WITH_TIMEOUT(timeout, ...)                               \
  do {                                                           \
    if (!WithTimeout(timeout, [=]() mutable { __VA_ARGS__; })) { \
      return;                                                    \
    }                                                            \
  } while (0)
// clang-format on

/**
 * @brief Try to execute f in a separate thread, leaking the execution of f if it takes too long
 * @param timeout The time to wait
 * @param f The function to call
 */
bool WithTimeout(std::chrono::milliseconds timeout, Function<void()> f) {
  auto promise = std::make_shared<std::promise<void>>();

  std::thread thread([promise, f = std::move(f)]() mutable {
    try {
      f();
      promise->set_value();
    } catch (...) {
      promise->set_exception(std::current_exception());
    }
  });

  auto future = promise->get_future();
  bool success = future.wait_for(timeout) == std::future_status::ready;

  EXPECT_TRUE(success) << "Function blocked for more than " << timeout.count() << " milliseconds, leaking pointer";

  if (success) {
    thread.join();
    future.get();
  } else {
    thread.detach();
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
  WITH_TIMEOUT(TIMEOUT, delete pool);
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
  WITH_TIMEOUT(TIMEOUT, delete pool);
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
  WITH_TIMEOUT(TIMEOUT, delete join_set);
  WITH_TIMEOUT(TIMEOUT, delete pool);
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
  WITH_TIMEOUT(TIMEOUT, delete join_set);
  WITH_TIMEOUT(TIMEOUT, delete pool);
}

TEST_F(ThreadPoolTest, JoinSetWaitsForCompletion) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  auto count = std::make_shared<std::atomic<int>>();

  ThreadPool::JoinSet *join_set = new ThreadPool::JoinSet(*pool);
  for (int i = 0; i < 5; ++i) {
    join_set->Spawn([count]() mutable {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      count->fetch_add(1);
    });
  }
  WITH_TIMEOUT(TIMEOUT, join_set->Wait());

  // Ensure all tasks have finished
  EXPECT_EQ(count->load(), 5);

  WITH_TIMEOUT(TIMEOUT, delete join_set);
  WITH_TIMEOUT(TIMEOUT, delete pool);
}

TEST_F(ThreadPoolTest, JoinSetDestructorWaitsForCompletion) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  auto count = std::make_shared<std::atomic<int>>();

  ThreadPool::JoinSet *join_set = new ThreadPool::JoinSet(*pool);
  for (int i = 0; i < 5; ++i) {
    join_set->Spawn([count]() mutable {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      count->fetch_add(1);
    });
  }
  WITH_TIMEOUT(TIMEOUT, delete join_set);

  // Ensure all tasks have finished
  EXPECT_EQ(count->load(), 5);

  WITH_TIMEOUT(TIMEOUT, delete pool);
}

TEST_F(ThreadPoolTest, JoinSetExceptionHandling) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  auto count = std::make_shared<std::atomic<int>>();

  ThreadPool::JoinSet *join_set = new ThreadPool::JoinSet(*pool);
  join_set->Spawn([]() mutable { throw std::runtime_error("expected"); });

  for (int i = 0; i < 5; ++i) {
    join_set->Spawn([count]() mutable {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      count->fetch_add(1);
    });
  }

  EXPECT_THROW(WITH_TIMEOUT(TIMEOUT, join_set->Wait()), std::runtime_error);

  EXPECT_LT(count->load(), 5);

  WITH_TIMEOUT(TIMEOUT, delete join_set);

  // Ensure all tasks have finished
  ASSERT_EQ(count->load(), 5);

  WITH_TIMEOUT(TIMEOUT, delete pool);
}

TEST_F(ThreadPoolTest, JoinSetMultipleWait) {
  ThreadPool *pool = new ThreadPool(3, *logger_);

  auto bitset = std::make_shared<std::atomic<int>>();

  ThreadPool::JoinSet *join_set_1 = new ThreadPool::JoinSet(*pool);
  ThreadPool::JoinSet *join_set_2 = new ThreadPool::JoinSet(*pool);
  ThreadPool::JoinSet *join_set_3 = new ThreadPool::JoinSet(*pool);

  join_set_1->Spawn([bitset]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    *bitset |= 1;
  });
  join_set_2->Spawn([bitset]() mutable { *bitset |= 2; });
  join_set_3->Spawn([bitset]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    *bitset |= 4;
  });

  WITH_TIMEOUT(TIMEOUT, delete join_set_2);
  EXPECT_EQ(bitset->load(), 2);

  WITH_TIMEOUT(TIMEOUT, delete join_set_3);
  EXPECT_EQ(bitset->load(), 6);

  WITH_TIMEOUT(TIMEOUT, delete join_set_1);
  EXPECT_EQ(bitset->load(), 7);

  WITH_TIMEOUT(TIMEOUT, delete pool);
}

TEST_F(ThreadPoolTest, ThreadPoolDestructorWaitsForCompletion) {
  ThreadPool *pool = new ThreadPool(2, *logger_);
  auto count = std::make_shared<std::atomic<int>>();

  for (int i = 0; i < 8; ++i) {
    pool->Spawn([count]() mutable {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      count->fetch_add(1);
    });
  }

  WITH_TIMEOUT(TIMEOUT, delete pool);

  // Ensure all tasks have finished
  ASSERT_EQ(count->load(), 8);
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
  WITH_TIMEOUT(TIMEOUT, delete pool);
}
