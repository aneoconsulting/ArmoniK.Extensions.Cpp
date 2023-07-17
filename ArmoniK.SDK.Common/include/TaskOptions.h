#ifndef ARMONIK_SDK_TASKOPTIONS_H
#define ARMONIK_SDK_TASKOPTIONS_H

#include "Duration.h"
#include <map>
#include <memory>
#include <string>

namespace armonik::api::grpc::v1 {
class TaskOptions;
}

namespace SDK_COMMON_NAMESPACE {
struct TaskOptions {
  std::string application_name;
  std::string application_version;
  std::string application_namespace;
  std::string application_service;
  std::string partition_id;
  std::string engine_type;
  int priority;
  int max_retries;
  Duration max_duration;
  std::map<std::string, std::string> options;

  TaskOptions(std::string applicationName, std::string applicationVersion, std::string applicationNamespace,
              std::string applicationService, std::string partitionId = "", std::string engineType = "",
              int priority = 2, int maxRetries = 3, const Duration &maxDuration = Duration(300, 0),
              const std::map<std::string, std::string> &options = std::map<std::string, std::string>());

  explicit TaskOptions(const armonik::api::grpc::v1::TaskOptions &raw);

  explicit operator armonik::api::grpc::v1::TaskOptions() const;
};
} // namespace SDK_COMMON_NAMESPACE
#endif // ARMONIK_SDK_TASKOPTIONS_H
