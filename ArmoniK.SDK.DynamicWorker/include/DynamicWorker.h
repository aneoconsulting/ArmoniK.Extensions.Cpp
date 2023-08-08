#ifndef ARMONIK_SDK_DYNAMICWORKER_H
#define ARMONIK_SDK_DYNAMICWORKER_H

#include "ApplicationManager.h"
#include <armonik/sdk/common/Configuration.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>

namespace SDK_DYNAMICWORKER_NAMESPACE {
/**
 * @brief ArmoniK Worker that loads a dynamic library and executes method within it
 */
class DynamicWorker : public ArmoniK::Api::Worker::ArmoniKWorker {
public:
  /**
   * @brief Creates a dynamic worker
   * @param agent Stub to communicate with the scheduling agent
   * @param config Configuration
   */
  explicit DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                         const ArmoniK::Sdk::Common::Configuration &config);

  /**
   * @brief Executes the task given by the task handler
   * @param taskHandler Task handler
   * @return Whether the task executed successfully or not
   */
  Api::Worker::ProcessStatus Execute(Api::Worker::TaskHandler &taskHandler) override;

private:
  /**
   * @brief Application manager
   */
  SDK_DYNAMICWORKER_NAMESPACE::ApplicationManager manager;
};
} // namespace SDK_DYNAMICWORKER_NAMESPACE

#endif // ARMONIK_SDK_DYNAMICWORKER_H
