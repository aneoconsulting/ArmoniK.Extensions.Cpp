#ifndef ARMONIK_SDK_TASKREQUEST_H
#define ARMONIK_SDK_TASKREQUEST_H

#include <sstream>
#include <vector>

namespace SDK_CLIENT_NAMESPACE {

    struct TaskRequest {
      TaskRequest(std::string method_name_, std::string arguments_,
                  std::vector<std::string> data_dependencies_ = std::vector<std::string>())
          : method_name(std::move(method_name_)), arguments(std::move(arguments_)),
            data_dependencies(std::move(data_dependencies_)) {}
      std::string method_name;
      std::string arguments;
      std::vector<std::string> data_dependencies;

      static std::string Serialize(const TaskRequest& request);

    };

    std::string TaskRequest::Serialize(const TaskRequest& request) {
        std::stringstream ss;

    }

} // namespace SDK_CLIENT_NAMESPACE

#endif // ARMONIK_SDK_TASKREQUEST_H
