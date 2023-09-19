#pragma once

#include <absl/strings/string_view.h>
#include <sstream>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief Task definition to submit
 */
struct TaskPayload {
  TaskPayload() = default;
  /**
   * @brief Constructs a task payload
   * @param method_name_ Method name
   * @param arguments_ Serialized arguments for the method
   * @param data_dependencies_ Data dependencies for the task. Empty by default
   */
  TaskPayload(std::string method_name_, std::string arguments_,
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
   * @brief Serializes the request into an ArmoniK compatible format
   * @return Serialized task request
   */
  [[nodiscard]] std::string Serialize() const;

  /**
   * @brief Deserializes a request from the ArmoniK compatible format
   * @param serialized Serialized task request
   * @return Deserialized task request
   */
  static TaskPayload Deserialize(absl::string_view serialized);
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
