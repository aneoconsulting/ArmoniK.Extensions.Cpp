#include "DynamicWorker.h"
#include "ApplicationManager.h"
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/DynamicLibrary.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <exception>
#include <fstream>

namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {

DynamicWorker::DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                             const ArmoniK::Sdk::Common::Configuration &config,
                             const armonik::api::common::logger::Logger &logger)
    : ArmoniKWorker(std::move(agent)), logger(logger.local({{"WorkerName", "DynamicWorker"}})),
      manager(config, logger) {}

armonik::api::worker::ProcessStatus DynamicWorker::Execute(armonik::api::worker::TaskHandler &taskHandler) {
  try {
    const auto &rawOptions = taskHandler.getTaskOptions();

    // Convention path: ConventionVersion key present in task options
    if (rawOptions.options().count(ArmoniK::Sdk::Common::DynamicLibrary::KeyConventionVersion)) {
      ArmoniK::Sdk::Common::TaskOptions opts(rawOptions);
      const auto version = opts.GetConventionVersion();
      if (version != ArmoniK::Sdk::Common::DynamicLibrary::ConventionVersion) {
        throw ArmoniK::Sdk::Common::ArmoniKSdkException("Unsupported convention version: " + version);
      }
      auto lib = opts.GetDynamicLibrary();
      const auto &deps = taskHandler.getDataDependencies();

      // Blob-based loading: fetch the .so content from data dependencies, write to a temp file,
      // then dlopen it. The library is uploaded as a blob by the client and pre-downloaded by
      // ArmoniK before the worker runs.
      if (!lib.library_blob_id.empty()) {
        const auto blob_it = deps.find(lib.library_blob_id);
        if (blob_it == deps.end()) {
          throw ArmoniK::Sdk::Common::ArmoniKSdkException("Library blob '" + lib.library_blob_id +
                                                          "' not found in data dependencies");
        }
        // Write to a deterministic temp path so identical blobs are only written once per worker process
        lib.library_path = "/tmp/armonik-lib-" + lib.library_blob_id + ".so";
        std::ofstream tmp(lib.library_path, std::ios::binary | std::ios::trunc);
        if (!tmp) {
          throw ArmoniK::Sdk::Common::ArmoniKSdkException("Failed to write library to temp file: " + lib.library_path);
        }
        tmp.write(blob_it->second.data(), static_cast<std::streamsize>(blob_it->second.size()));
      }

      const auto payload = ArmoniK::Sdk::Common::ConventionPayload::Deserialize(taskHandler.getPayload());

      if (lib.symbol.empty()) {
        throw ArmoniK::Sdk::Common::ArmoniKSdkException(
            "Convention task has no method name: set the 'Symbol' task option");
      }
      const std::string &method_name = lib.symbol;

      // Resolve inputs: if a value matches a data dependency key (blob ID), substitute its downloaded content.
      // This handles both inline values (C++ native payloads) and blob ID references (cross-SDK interoperability).
      std::map<std::string, std::string> resolved_inputs;
      for (const auto &[name, value] : payload.inputs) {
        const auto dep_it = deps.find(value);
        resolved_inputs[name] = (dep_it != deps.end()) ? dep_it->second : value;
      }

      return manager.UseLibrary(lib, rawOptions.application_namespace(), rawOptions.application_service())
          .UseSession(taskHandler.getSessionId())
          .Execute(taskHandler, method_name, resolved_inputs, payload.outputs);
    }

    // Legacy path: use application_name / application_version based loading
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto legacyPayload = ArmoniK::Sdk::Common::TaskPayload::Deserialize(taskHandler.getPayload());
#pragma GCC diagnostic pop
    AppId appId{rawOptions.application_name(), rawOptions.application_version()};
    ServiceId serviceId(appId, rawOptions.application_namespace(), rawOptions.application_service());
    return manager.UseApplication(appId)
        .UseService(serviceId)
        .UseSession(taskHandler.getSessionId())
        .Execute(taskHandler, legacyPayload.method_name, legacyPayload.arguments);
  } catch (const ArmoniK::Sdk::Common::ArmoniKSdkException &e) {
    return armonik::api::worker::ProcessStatus(e.what());
  }
}

} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
