#pragma once
#include <algorithm>
#include <cctype>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Common {
inline std::string to_lower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
  return str;
}
inline std::string to_capitalized(std::string str) {
  if (str.empty()) {
    return str;
  }
  str[0] = (char)std::toupper(str[0]);
  std::transform(str.begin() + 1, str.end(), str.begin() + 1, [](unsigned char c) { return std::tolower(c); });
  return str;
}
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
