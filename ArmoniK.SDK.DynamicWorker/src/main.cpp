#include "DynamicWorker.h"
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <armonik/worker/utils/WorkerServer.h>

int main() {
  std::cout << "Starting ArmoniK SDK worker" << std::endl;

  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  try {
    ArmoniK::Api::Worker::WorkerServer::create<SDK_DYNAMICWORKER_NAMESPACE::DynamicWorker>(
        ArmoniK::Api::Common::utils::Configuration(config), config)
        ->run();
  } catch (const std::exception &e) {
    std::cout << "Error in worker" << e.what() << std::endl;
  }

  std::cout << "Stopping ArmoniK SDK worker" << std::endl;
  return 0;
}
