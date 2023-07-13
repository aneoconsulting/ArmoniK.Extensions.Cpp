#ifndef ARMONIK_SDK_TASKOPTIONS_H
#define ARMONIK_SDK_TASKOPTIONS_H

#include <memory>
#include <string>
#include <map>
#include "Duration.h"

namespace SDK_COMMON_NAMESPACE{
    struct TaskOptions{
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

        TaskOptions(std::string applicationName, std::string applicationVersion,
                    std::string applicationNamespace, std::string applicationService,
                    std::string partitionId, std::string engineType, int priority, int maxRetries,
                    const Duration &maxDuration, const std::map<std::string, std::string> &options);
    };
}
#endif // ARMONIK_SDK_TASKOPTIONS_H
