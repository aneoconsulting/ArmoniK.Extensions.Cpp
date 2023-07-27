#ifndef ARMONIK_SDK_SDKWORKER_H
#define ARMONIK_SDK_SDKWORKER_H

#include <armonik/sdk/worker/ApplicationManager.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>

namespace SDK_DLLWORKER_NAMESPACE {
class SDKWorker : public ArmoniK::Api::Worker::ArmoniKWorker {
public:
  explicit SDKWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent);
  Api::Worker::ProcessStatus Execute(Api::Worker::TaskHandler &taskHandler) override;

private:
  ArmoniK::SDK::Worker::ApplicationManager manager;
};
} // namespace SDK_DLLWORKER_NAMESPACE

#endif // ARMONIK_SDK_SDKWORKER_H
