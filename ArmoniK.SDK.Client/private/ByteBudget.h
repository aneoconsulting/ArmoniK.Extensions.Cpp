#pragma once

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

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
