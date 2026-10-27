#pragma once

#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Internal {

/**
 * @brief Encodes task options as JSON, to pass them through the C interface of worker libraries (armonik_task_t)
 *
 * @note This is an internal SDK header. It is not part of the public API.
 */
inline std::string TaskOptionsToJson(const Common::TaskOptions &options) {
  return nlohmann::json{{"application_name", options.application_name},
                        {"application_version", options.application_version},
                        {"application_namespace", options.application_namespace},
                        {"application_service", options.application_service},
                        {"partition_id", options.partition_id},
                        {"engine_type", options.engine_type},
                        {"priority", options.priority},
                        {"max_retries", options.max_retries},
                        {"max_duration",
                         {{"seconds", options.max_duration.seconds}, {"nanos", options.max_duration.nanos}}},
                        {"options", options.options}}
      .dump();
}

/**
 * @brief Decodes task options encoded by TaskOptionsToJson
 * @throws ArmoniKSdkException if the JSON is invalid
 */
inline Common::TaskOptions TaskOptionsFromJson(const char *data, std::size_t size) {
  try {
    const auto j = nlohmann::json::parse(data, data + size);
    const auto &duration = j.at("max_duration");
    return Common::TaskOptions(
        j.at("application_name").get<std::string>(), j.at("application_version").get<std::string>(),
        j.at("application_namespace").get<std::string>(), j.at("application_service").get<std::string>(),
        j.at("partition_id").get<std::string>(), j.at("engine_type").get<std::string>(), j.at("priority").get<int>(),
        j.at("max_retries").get<int>(),
        Common::Duration(duration.at("seconds").get<int64_t>(), duration.at("nanos").get<int32_t>()),
        j.at("options").get<std::map<std::string, std::string>>());
  } catch (const nlohmann::json::exception &e) {
    throw Common::ArmoniKSdkException(std::string("Invalid task options: ") + e.what());
  }
}

} // namespace Internal
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
