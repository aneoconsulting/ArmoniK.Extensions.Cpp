#include "DynamicWorker.h"
#include "ApplicationManager.h"
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/DynamicLibrary.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <exception>

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
      const auto lib = opts.GetDynamicLibrary();
      const auto payload = ArmoniK::Sdk::Common::TaskPayload::Deserialize(taskHandler.getPayload());
      return manager.UseLibrary(lib)
          .UseSession(taskHandler.getSessionId())
          .Execute(taskHandler, payload.method_name, payload.inputs, payload.outputs);
    }

    // Legacy path: use application_name / application_version based loading
    auto legacyPayload = ArmoniK::Sdk::Common::LegacyTaskPayload::Deserialize(taskHandler.getPayload());
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
