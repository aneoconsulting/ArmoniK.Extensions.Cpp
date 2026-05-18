#include "armonik/sdk/common/TaskOptions.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"

#include <armonik/common/objects.pb.h>

using namespace ArmoniK::Sdk::Common;

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
      max_retries(raw.max_retries()), max_duration{raw.max_duration().seconds(), raw.max_duration().nanos()},
      options(raw.options().begin(), raw.options().end()) {}

void TaskOptions::SetDynamicLibrary(const DynamicLibrary &lib) {
  options[DynamicLibrary::KeyConventionVersion] = DynamicLibrary::ConventionVersion;
  options[DynamicLibrary::KeyLibraryPath] = lib.library_path;
  options[DynamicLibrary::KeySymbol] = lib.symbol;
  if (!lib.library_blob_id.empty()) {
    options[DynamicLibrary::KeyLibraryBlobId] = lib.library_blob_id;
  }
}

DynamicLibrary TaskOptions::GetDynamicLibrary() const {
  DynamicLibrary lib;

  // LibraryBlobId: optional — when set, library_path is resolved at runtime from blob storage
  auto it = options.find(DynamicLibrary::KeyLibraryBlobId);
  if (it != options.end()) {
    lib.library_blob_id = it->second;
  }

  // LibraryPath: required only when LibraryBlobId is not set
  it = options.find(DynamicLibrary::KeyLibraryPath);
  if (it == options.end() || it->second.empty()) {
    if (lib.library_blob_id.empty()) {
      throw ArmoniKSdkException(std::string("Missing key '") + DynamicLibrary::KeyLibraryPath +
                                "' in task options (required when '" + DynamicLibrary::KeyLibraryBlobId +
                                "' is not set)");
    }
    // library_path will be resolved at runtime from the blob content
  } else {
    lib.library_path = it->second;
  }

  it = options.find(DynamicLibrary::KeySymbol);
  if (it != options.end()) {
    lib.symbol = it->second;
  }
  return lib;
}

std::string TaskOptions::GetConventionVersion() const {
  auto it = options.find(DynamicLibrary::KeyConventionVersion);
  if (it == options.end()) {
    throw ArmoniKSdkException(std::string("Missing key '") + DynamicLibrary::KeyConventionVersion +
                              "' in task options");
  }
  return it->second;
}
