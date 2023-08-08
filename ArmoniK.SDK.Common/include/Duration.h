#ifndef ARMONIK_SDK_DURATION_H
#define ARMONIK_SDK_DURATION_H

#include <cstdint>

namespace SDK_COMMON_NAMESPACE {
/**
 * @brief Simple duration structure
 */
struct Duration {
  /**
   * @brief Seconds
   */
  int64_t seconds = 0;
  /**
   * @brief Nanoseconds
   */
  int32_t nanos = 0;
};
} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_DURATION_H
