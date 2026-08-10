#pragma once

#include <cstdint>

namespace ArmoniK {
namespace Sdk {
namespace Common {
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

  Duration() = default;
  Duration(int64_t seconds, int32_t nanos) : seconds(seconds), nanos(nanos) {}
};
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
