#pragma once

#include <absl/strings/string_view.h>
#include <map>
#include <sstream>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief Legacy task payload using custom binary encoding.
 * Used on the legacy execution path (application_name / application_version based loading).
 */
struct LegacyTaskPayload {
  LegacyTaskPayload() = default;
  /**
   * @brief Constructs a legacy task payload
   * @param method_name_ Method name
   * @param arguments_ Serialized arguments for the method
   * @param data_dependencies_ Data dependencies for the task. Empty by default
   */
  LegacyTaskPayload(std::string method_name_, std::string arguments_,
                    std::vector<std::string> data_dependencies_ = std::vector<std::string>())
      : method_name(std::move(method_name_)), arguments(std::move(arguments_)),
        data_dependencies(std::move(data_dependencies_)) {}
  /**
   * @brief Task's method name
   */
  std::string method_name;

  /**
   * @brief Method's serialized arguments
   */
  std::string arguments;

  /**
   * @brief Task's data dependencies
   */
  std::vector<std::string> data_dependencies;

  /**
   * @brief Serializes the payload into the legacy binary format
   * @return Serialized payload
   */
  [[nodiscard]] std::string Serialize() const;

  /**
   * @brief Deserializes a payload from the legacy binary format
   * @param serialized Serialized payload
   * @return Deserialized payload
   */
  static LegacyTaskPayload Deserialize(absl::string_view serialized);
};

/**
 * @brief Convention task payload using JSON encoding.
 * Used on the convention execution path (LibraryPath / Symbol based loading).
 *
 * Serialized format: {"method":"<method_name>","inputs":{...},"outputs":{...}}
 */
struct TaskPayload {
  TaskPayload() = default;

  /**
   * @brief Method name to dispatch to
   */
  std::string method_name;

  /**
   * @brief Named inputs: maps user-defined name to blob ID
   */
  std::map<std::string, std::string> inputs;

  /**
   * @brief Named outputs: maps user-defined name to pre-allocated blob ID
   */
  std::map<std::string, std::string> outputs;

  /**
   * @brief Serializes the payload to JSON
   * @return JSON string
   */
  [[nodiscard]] std::string Serialize() const;

  /**
   * @brief Deserializes the payload from JSON
   * @param serialized JSON string
   * @return Deserialized payload
   */
  static TaskPayload Deserialize(absl::string_view serialized);
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
