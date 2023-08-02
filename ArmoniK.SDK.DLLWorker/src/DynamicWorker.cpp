#include "DynamicWorker.h"
#include <ArmoniKSdkException.h>
#include <TaskPayload.h>
#include <armonik/sdk/worker/ApplicationManager.h>

SDK_DLLWORKER_NAMESPACE::DynamicWorker::DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                                                      const ArmoniK::Sdk::Common::Configuration &config)
    : ArmoniKWorker(std::move(agent)), manager(config) {}

ArmoniK::Api::Worker::ProcessStatus

SDK_DLLWORKER_NAMESPACE::DynamicWorker::Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler) {
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
  } catch (const ArmoniK::Sdk::Common::ArmoniKSdkException &e) {
    return ArmoniK::Api::Worker::ProcessStatus(e.what());
  }

  return ArmoniK::Api::Worker::ProcessStatus::Ok;
}
