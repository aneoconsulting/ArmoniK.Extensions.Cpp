#pragma once

#include <armonik/common/utils/string_view.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Common {
namespace internal {

/**
 * @brief Fixed-width hexadecimal encoding of the field sizes in the legacy TaskPayload format.
 *
 * @note This is an internal SDK header. It is not part of the public API and
 *       may change or be removed in any future release without notice.
 */
template <typename T> struct TypeParseTraits;

template <> struct TypeParseTraits<std::uint32_t> {
  static const char *name() { return "uint32_t"; }
};

/**
 * @brief Writes `i` as hexadecimal, zero-padded to 2 * sizeof(T) digits.
 */
template <typename T> std::string int_to_hex(T i) {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
  return stream.str();
}

/**
 * @brief Parses the whole of `str` as a hexadecimal value of type T.
 * @throws std::runtime_error if `str` is not hexadecimal, or its value does not fit in T.
 */
template <typename T> T hex_to_int(armonik::api::string_view str) {
  char *endPos;
  std::string null_terminated(str.data(), str.size());
  // strtoull, not strtol: long is 32 bits on Windows, so strtol rejects field sizes >= 2^31.
  errno = 0;
  unsigned long long value = std::strtoull(null_terminated.data(), &endPos, 16);
  if (errno == ERANGE || value > (std::numeric_limits<T>::max)()) {
    throw std::runtime_error(null_terminated + " is too large for " + TypeParseTraits<T>::name());
  }
  if (endPos != null_terminated.data() + null_terminated.size()) {
    throw std::runtime_error(null_terminated + " is not convertible to " + TypeParseTraits<T>::name());
  }
  return static_cast<T>(value);
}

} // namespace internal
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
