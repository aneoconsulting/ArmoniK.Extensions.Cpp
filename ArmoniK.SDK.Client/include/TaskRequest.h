#ifndef ARMONIK_SDK_TASKREQUEST_H
#define ARMONIK_SDK_TASKREQUEST_H

namespace SDK_CLIENT_NAMESPACE {

struct TaskRequest {
  TaskRequest(std::string method_name_, std::vector<uint8_t> payload_,
              std::vector<std::string> data_dependencies_ = std::vector<std::string>())
      : method_name(std::move(method_name_)), payload(std::move(payload_)),
        data_dependencies(std::move(data_dependencies_)) {}
  std::string method_name;
  std::vector<uint8_t> payload;
  std::vector<std::string> data_dependencies;
};

} // namespace SDK_CLIENT_NAMESPACE

#endif // ARMONIK_SDK_TASKREQUEST_H
