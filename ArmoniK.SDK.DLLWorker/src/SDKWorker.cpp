#include "SDKWorker.h"
#include <ArmoniKSDKException.h>
#include <TaskPayload.h>
#include <armonik/sdk/worker/ApplicationManager.h>

SDK_DLLWORKER_NAMESPACE::SDKWorker::SDKWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent)
    : ArmoniKWorker(std::move(agent)), manager() {}

ArmoniK::Api::Worker::ProcessStatus

SDK_DLLWORKER_NAMESPACE::SDKWorker::Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler) {
  try {
    auto &payload = taskHandler.getPayload();
    auto taskPayload = ArmoniK::Sdk::Common::TaskPayload::Deserialize(payload);
    ArmoniK::Sdk::Worker::AppId appId(taskHandler.getTaskOptions().application_name(),
                                      taskHandler.getTaskOptions().application_version());
    ArmoniK::Sdk::Worker::ServiceId serviceId(appId, taskHandler.getTaskOptions().application_namespace(),
                                              taskHandler.getTaskOptions().application_service());
    manager.UseApplication(appId)
        .UseService(serviceId)
        .UseSession(taskHandler.getSessionId())
        .Execute(taskHandler, taskPayload.method_name, taskPayload.arguments);
  } catch (const ArmoniK::Sdk::Common::ArmoniKSDKException &e) {
    return ArmoniK::Api::Worker::ProcessStatus(e.what());
  }

  return ArmoniK::Api::Worker::ProcessStatus::Ok;
}
