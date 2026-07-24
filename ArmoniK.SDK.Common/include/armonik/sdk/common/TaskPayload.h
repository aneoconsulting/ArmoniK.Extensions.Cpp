#pragma once

#include <armonik/common/utils/string_view.h>
#include <map>
#include <sstream>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief Task payload using custom binary encoding.
 * Used on the legacy execution path (application_name / application_version based loading).
 * @deprecated Use TaskDefinition with Submit(std::vector<TaskDefinition>) instead.
 */
struct [[deprecated("Use TaskDefinition with Submit(std::vector<TaskDefinition>) instead")]] TaskPayload {
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
   * @brief Serializes the payload into the legacy binary format
   * @return Serialized payload
   */
  [[nodiscard]] std::string Serialize() const;

  /**
   * @brief Deserializes a payload from the legacy binary format
   * @param serialized Serialized payload
   * @return Deserialized payload
   */
  static TaskPayload Deserialize(armonik::api::string_view serialized);
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
