#include "DynamicWorker.h"
#include "ApplicationManager.h"
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/TaskPayload.h>
namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {

DynamicWorker::DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                             const ArmoniK::Sdk::Common::Configuration &config)
    : ArmoniKWorker(std::move(agent)), manager(config) {}

armonik::api::worker::ProcessStatus

DynamicWorker::Execute(armonik::api::worker::TaskHandler &taskHandler) {
  try {
    auto &payload = taskHandler.getPayload();
    auto taskPayload = ArmoniK::Sdk::Common::TaskPayload::Deserialize(payload);
    AppId appId{taskHandler.getTaskOptions().application_name(), taskHandler.getTaskOptions().application_version()};
    ServiceId serviceId(appId, taskHandler.getTaskOptions().application_namespace(),
                        taskHandler.getTaskOptions().application_service());
    manager.UseApplication(appId)
        .UseService(serviceId)
        .UseSession(taskHandler.getSessionId())
        .Execute(taskHandler, taskPayload.method_name, taskPayload.arguments);
  } catch (const ArmoniK::Sdk::Common::ArmoniKSdkException &e) {
    return armonik::api::worker::ProcessStatus(e.what());
  }

  return armonik::api::worker::ProcessStatus::Ok;
}
} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
