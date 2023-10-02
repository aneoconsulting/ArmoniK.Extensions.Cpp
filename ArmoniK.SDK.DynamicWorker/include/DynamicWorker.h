#pragma once

#include "ApplicationManager.h"
#include <armonik/common/logger/local_logger.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>

namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {
/**
 * @brief ArmoniK Worker that loads a dynamic library and executes method within it
 */
class DynamicWorker : public armonik::api::worker::ArmoniKWorker {
public:
  /**
   * @brief Creates a dynamic worker
   * @param agent Stub to communicate with the scheduling agent
   * @param config Configuration
   */
  explicit DynamicWorker(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent,
                         const ArmoniK::Sdk::Common::Configuration &config,
                         const armonik::api::common::logger::Logger &logger);

  /**
   * @brief Executes the task given by the task handler
   * @param taskHandler Task handler
   * @return Whether the task executed successfully or not
   */
  armonik::api::worker::ProcessStatus Execute(armonik::api::worker::TaskHandler &taskHandler) override;

private:
  /**
   * @brief Local logger
   */
  armonik::api::common::logger::LocalLogger logger;

  /**
   * @brief Application manager
   */
  ArmoniK::Sdk::DynamicWorker::ApplicationManager manager;
};
} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
