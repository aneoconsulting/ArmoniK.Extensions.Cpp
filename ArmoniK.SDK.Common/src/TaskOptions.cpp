#include "armonik/sdk/common/TaskOptions.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"

#include <armonik/common/objects.pb.h>

using namespace ArmoniK::Sdk::Common;

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

void TaskOptions::SetConvention(const std::string &symbol) {
  options[DynamicLibrary::KeyConventionVersion] = DynamicLibrary::ConventionVersion;
  options[DynamicLibrary::KeySymbol] = symbol;
}

void TaskOptions::SetDynamicLibrary(const DynamicLibrary &lib) {
  SetConvention(lib.symbol);
  options[DynamicLibrary::KeyLibraryPath] = lib.library_path;
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

std::string TaskOptions::GetSymbol() const {
  auto it = options.find(DynamicLibrary::KeySymbol);
  if (it == options.end() || it->second.empty()) {
    throw ArmoniKSdkException(std::string("Convention task has no method name: set the '") + DynamicLibrary::KeySymbol +
                              "' task option");
  }
  return it->second;
}

std::string TaskOptions::GetConventionVersion() const {
  auto it = options.find(DynamicLibrary::KeyConventionVersion);
  if (it == options.end()) {
    throw ArmoniKSdkException(std::string("Missing key '") + DynamicLibrary::KeyConventionVersion +
                              "' in task options");
  }
  return it->second;
}
