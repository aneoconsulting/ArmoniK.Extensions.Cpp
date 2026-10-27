#include "DynamicWorker.h"
#include "ApplicationManager.h"
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/ConventionTask.h>
#include <armonik/sdk/common/Diagnostics.h>
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
    const ArmoniK::Sdk::Common::TaskOptions opts(rawOptions);
    if (ArmoniK::Sdk::Common::ConventionTask::IsConventionTask(opts)) {
      const auto &deps = taskHandler.getDataDependencies();
      const auto task = ArmoniK::Sdk::Common::ConventionTask::Decode(opts, taskHandler.getPayload(), deps,
                                                                     taskHandler.getExpectedResults());
      auto lib = opts.GetDynamicLibrary();

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

      return manager.UseLibrary(lib, rawOptions.application_namespace(), rawOptions.application_service())
          .UseSession(taskHandler.getSessionId())
          .Execute(taskHandler, task);
    }

    // Legacy path: use application_name / application_version based loading
    ARMONIK_SDK_SUPPRESS_DEPRECATED_BEGIN
    auto legacyPayload = ArmoniK::Sdk::Common::TaskPayload::Deserialize(taskHandler.getPayload());
    ARMONIK_SDK_SUPPRESS_DEPRECATED_END
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
