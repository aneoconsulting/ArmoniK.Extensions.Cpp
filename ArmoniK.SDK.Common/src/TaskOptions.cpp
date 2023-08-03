#include "TaskOptions.h"

#include <armonik/common/objects.pb.h>

using namespace SDK_COMMON_NAMESPACE;

TaskOptions::TaskOptions(std::string applicationName, std::string applicationVersion, std::string applicationNamespace,
                         std::string applicationService, std::string partitionId, std::string engineType, int priority,
                         int maxRetries, const Duration &maxDuration, const std::map<std::string, std::string> &options)
    : application_name(std::move(applicationName)), application_version(std::move(applicationVersion)),
      application_namespace(std::move(applicationNamespace)), application_service(std::move(applicationService)),
      partition_id(std::move(partitionId)), engine_type(std::move(engineType)), priority(priority),
      max_retries(maxRetries), max_duration(maxDuration), options(options) {}

TaskOptions::operator armonik::api::grpc::v1::TaskOptions() const {
  armonik::api::grpc::v1::TaskOptions raw;
  raw.mutable_max_duration()->set_seconds(max_duration.seconds);
  raw.mutable_max_duration()->set_nanos(max_duration.nanos);
  raw.set_application_name(application_name);
  raw.set_application_namespace(application_namespace);
  raw.set_application_service(application_service);
  raw.set_application_version(application_version);
  raw.set_partition_id(partition_id);
  raw.set_engine_type(engine_type);
  raw.set_priority(priority);
  raw.set_max_retries(max_retries);
  raw.mutable_options()->insert(options.begin(), options.end());

  return raw;
}

TaskOptions::TaskOptions(const armonik::api::grpc::v1::TaskOptions &raw)
    : application_name(raw.application_name()), application_version(raw.application_version()),
      application_namespace(raw.application_namespace()), application_service(raw.application_service()),
      partition_id(raw.partition_id()), engine_type(raw.engine_type()), priority(raw.priority()),
      max_retries(raw.max_retries()), max_duration(raw.max_duration().seconds(), raw.max_duration().nanos()),
      options(raw.options().begin(), raw.options().end()) {}
