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
/**
 * @brief Simplified TaskOptions
 */
struct TaskOptions {
  /**
   * @brief Application name, used when loading the worker library
   */
  std::string application_name;
  /**
   * @brief Application version, used when loading the worker library
   */
  std::string application_version;
  /**
   * @brief Application namespace, used when creating the service of the worker library
   */
  std::string application_namespace;

  /**
   * @brief Application service, used when creating the service of the worker library
   */
  std::string application_service;

  /**
   * @brief Infrastructure partition to send the task to
   */
  std::string partition_id;

  /**
   * @brief SDK engine type (unused in the C++ SDK)
   */
  std::string engine_type;

  /**
   * @brief Task priority
   */
  int priority;

  /**
   * @brief Max number of retries for the task
   */
  int max_retries;

  /**
   * @brief Max task duration
   */
  Duration max_duration;

  /**
   * @brief User defined options map
   */
  std::map<std::string, std::string> options;

  /**
   * @brief Creates a task option
   * @param applicationName Application name, used when loading the worker library
   * @param applicationVersion Application version, used when loading the worker library
   * @param applicationNamespace Application namespace, used when creating the service of the worker library
   * @param applicationService Application service, used when creating the service of the worker library
   * @param partitionId Infrastructure partition to send the task to, defaults to ""
   * @param engineType SDK engine type (unused in the C++ SDK), defaults to ""
   * @param priority Task priority, defaults to 2
   * @param maxRetries Max number of retries for the task, defaults to 3
   * @param maxDuration Max task duration, defaults to 5 minutes
   * @param options User defined options map, defaults to an empty map
   */
  TaskOptions(std::string applicationName, std::string applicationVersion, std::string applicationNamespace,
              std::string applicationService, std::string partitionId = "", std::string engineType = "",
              int priority = 2, int maxRetries = 3, const Duration &maxDuration = Duration(300, 0),
              const std::map<std::string, std::string> &options = std::map<std::string, std::string>());

  /**
   * @brief Converts an ArmoniK compatible TaskOptions object to this TaskOptions object
   * @param raw ArmoniK compatible task options
   */
  explicit TaskOptions(const armonik::api::grpc::v1::TaskOptions &raw);

  /**
   * @brief Converts this TaskOptions object to an ArmoniK compatible object
   * @return ArmoniK compatible TaskOptions object
   */
  explicit operator armonik::api::grpc::v1::TaskOptions() const;
};
} // namespace SDK_COMMON_NAMESPACE
#endif // ARMONIK_SDK_TASKOPTIONS_H
