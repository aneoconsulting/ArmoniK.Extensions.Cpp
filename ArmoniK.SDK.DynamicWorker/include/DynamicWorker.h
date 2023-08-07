#ifndef ARMONIK_SDK_DYNAMICWORKER_H
#define ARMONIK_SDK_DYNAMICWORKER_H

#include "ApplicationManager.h"
#include <armonik/sdk/common/Configuration.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>

namespace SDK_DYNAMICWORKER_NAMESPACE {
class DynamicWorker : public ArmoniK::Api::Worker::ArmoniKWorker {
public:
  explicit DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                         const ArmoniK::Sdk::Common::Configuration &config);
  Api::Worker::ProcessStatus Execute(Api::Worker::TaskHandler &taskHandler) override;

private:
  SDK_DYNAMICWORKER_NAMESPACE::ApplicationManager manager;
};
} // namespace SDK_DYNAMICWORKER_NAMESPACE

#endif // ARMONIK_SDK_DYNAMICWORKER_H
