#pragma once

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

/**
 * @brief Bounds the total number of bytes admitted at once, blocking Acquire() until enough
 * capacity is free. Used to bound peak result-payload memory during WaitResults() independently
 * of thread_pool_'s size.
 */
class ByteBudget {
public:
  /**
   * @brief Construct a byte budget
   * @param capacity Maximum number of bytes that may be reserved at once. <= 0 disables the
   * budget: Acquire()/Release() become no-ops.
   */
  explicit ByteBudget(std::int64_t capacity) : capacity_(capacity), used_(0) {}

  ByteBudget(const ByteBudget &) = delete;
  ByteBudget &operator=(const ByteBudget &) = delete;

  /**
   * @brief Reserve `bytes` from the budget, blocking until there is room.
   * @param bytes Number of bytes to reserve. <= 0 is a no-op.
   * @return true if this single request alone exceeds the whole budget and was admitted anyway
   * because nothing else was reserved to drain first (callers can use this to warn), false
   * otherwise.
   */
  bool Acquire(std::int64_t bytes) {
    if (capacity_ <= 0 || bytes <= 0) {
      return false;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    bool oversized = false;
    cv_.wait(lock, [&] {
      if (used_ == 0 && bytes > capacity_) {
        // Nothing else reserved and this single request alone exceeds the budget: waiting
        // further would block forever without ever freeing room. Let it through instead.
        oversized = true;
        return true;
      }
      return used_ + bytes <= capacity_;
    });
    used_ += bytes;
    return oversized;
  }

  /**
   * @brief Release `bytes` previously reserved via Acquire().
   * @param bytes Number of bytes to release. <= 0 is a no-op.
   */
  void Release(std::int64_t bytes) {
    if (capacity_ <= 0 || bytes <= 0) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      used_ -= bytes;
    }
    cv_.notify_all();
  }

  /**
   * @brief The configured capacity, in bytes. <= 0 means the budget is disabled.
   */
  [[nodiscard]] std::int64_t capacity() const { return capacity_; }

private:
  std::int64_t capacity_;
  std::int64_t used_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

/**
 * @brief Bounds how many callers may hold a permit at once, blocking Acquire() until one frees up.
 *
 * Caps concurrent Submit()/SubmitRaw() calls to thread_pool_'s worker count minus one. A result
 * handler chaining a new Submit() from inside WaitResults() runs on a thread_pool_ worker, and that
 * call can block reserving upload_byte_budget_; this cap keeps at least one worker free to run the
 * uploads that release the budget for whichever call currently holds it.
 *
 * Implemented as a ByteBudget reserved 1 unit at a time. Unlike a byte budget, a concurrency limit
 * must never be disabled, so the capacity is clamped to at least 1 instead of treating <= 0 as
 * "unbounded".
 */
class ConcurrencySemaphore {
public:
  /**
   * @brief Construct a semaphore
   * @param capacity Maximum number of permits that may be held at once. Clamped to at least 1.
   */
  explicit ConcurrencySemaphore(std::int64_t capacity) : budget_(std::max<std::int64_t>(capacity, 1)) {}

  ConcurrencySemaphore(const ConcurrencySemaphore &) = delete;
  ConcurrencySemaphore &operator=(const ConcurrencySemaphore &) = delete;

  /**
   * @brief Acquire a permit, blocking until one is free.
   */
  void Acquire() { budget_.Acquire(1); }

  /**
   * @brief Release a permit previously acquired via Acquire().
   */
  void Release() { budget_.Release(1); }

private:
  ByteBudget budget_;
};

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
