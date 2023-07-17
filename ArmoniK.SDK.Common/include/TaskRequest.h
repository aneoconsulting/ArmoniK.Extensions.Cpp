#ifndef ARMONIK_SDK_TASKREQUEST_H
#define ARMONIK_SDK_TASKREQUEST_H

#include <sstream>
#include <vector>

namespace SDK_COMMON_NAMESPACE {

struct TaskRequest {
  TaskRequest(std::string service_name_, std::string method_name_, std::string arguments_,
              std::vector<std::string> data_dependencies_ = std::vector<std::string>())
      : service_name(std::move(service_name_)), method_name(std::move(method_name_)), arguments(std::move(arguments_)),
        data_dependencies(std::move(data_dependencies_)) {}
  std::string service_name;
  std::string method_name;
  std::string arguments;
  std::vector<std::string> data_dependencies;

  [[nodiscard]] std::string Serialize() const;
  static TaskRequest Deserialize(std::string_view serialized);
};

} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_TASKREQUEST_H
