#ifndef ARMONIK_SDK_DYNAMICWORKER_H
#define ARMONIK_SDK_DYNAMICWORKER_H

#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/worker/ApplicationManager.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>

namespace SDK_DLLWORKER_NAMESPACE {
class DynamicWorker : public ArmoniK::Api::Worker::ArmoniKWorker {
public:
  explicit DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                         const ArmoniK::Sdk::Common::Configuration &config);
  Api::Worker::ProcessStatus Execute(Api::Worker::TaskHandler &taskHandler) override;

private:
  ArmoniK::Sdk::Worker::ApplicationManager manager;
};
} // namespace SDK_DLLWORKER_NAMESPACE

#endif // ARMONIK_SDK_DYNAMICWORKER_H
