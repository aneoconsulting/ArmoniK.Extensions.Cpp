#ifndef ARMONIK_SDK_TASKREQUEST_H
#define ARMONIK_SDK_TASKREQUEST_H

namespace SDK_CLIENT_NAMESPACE {

    struct TaskRequest{
        TaskRequest(std::string method_name_, std::vector<std::byte> payload_, std::vector<std::string> data_dependencies_) : method_name(std::move(method_name_)), payload(std::move(payload_)), data_dependencies(std::move(data_dependencies_)){}
        std::string method_name;
        std::vector<std::byte> payload;
        std::vector<std::string> data_dependencies;
    };

}

#endif //ARMONIK_SDK_TASKREQUEST_H
