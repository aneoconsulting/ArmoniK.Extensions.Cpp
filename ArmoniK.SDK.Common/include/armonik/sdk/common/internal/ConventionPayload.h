#pragma once

#include <absl/strings/string_view.h>
#include <map>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief Convention task payload using JSON encoding.
 *
 * Internal wire format for the convention execution path.
 * Serialized format: {"method":"<method_name>","inputs":{...},"outputs":{...}}
 *
 * @note This is an internal SDK type. It is not part of the public API and
 *       may change or be removed in any future release without notice.
 */
struct ConventionPayload {
  ConventionPayload() = default;

  std::string method_name;
  std::map<std::string, std::string> inputs;
  std::map<std::string, std::string> outputs;

  [[nodiscard]] std::string Serialize() const;
  static ConventionPayload Deserialize(absl::string_view serialized);
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
