#include "TaskOptions.h"

using namespace SDK_COMMON_NAMESPACE;

TaskOptions::TaskOptions(std::string applicationName, std::string applicationVersion,
                         std::string applicationNamespace, std::string applicationService,
                         std::string partitionId = "", std::string engineType = "", int priority = 2, int maxRetries=3,
                         const Duration &maxDuration = Duration(300, 0), const std::map<std::string, std::string> &options = std::map<std::string, std::string>())
        : application_name(std::move(applicationName)), application_version(std::move(applicationVersion)),
          application_namespace(std::move(applicationNamespace)), application_service(std::move(applicationService)),
          partition_id(std::move(partitionId)), engine_type(std::move(engineType)), priority(priority), max_retries(maxRetries),
          max_duration(maxDuration), options(options) {}
