#ifndef ARMONIK_SDK_TASKPAYLOAD_H
#define ARMONIK_SDK_TASKPAYLOAD_H

#include <sstream>
#include <vector>

namespace SDK_COMMON_NAMESPACE {

struct TaskPayload {
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
  static TaskPayload Deserialize(std::string_view serialized);
};

} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_TASKPAYLOAD_H
