#include "DynamicWorker.h"
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <armonik/worker/utils/WorkerServer.h>

int main() {
  ArmoniK::Sdk::Common::Configuration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  armonik::api::common::logger::Logger logger{armonik::api::common::logger::writer_console(),
                                              armonik::api::common::logger::formatter_clef(), config.get_log_level()};

  logger.log(armonik::api::common::logger::Level::Info, "Starting ArmoniK SDK worker");

  try {
    armonik::api::worker::WorkerServer::create<ArmoniK::Sdk::DynamicWorker::DynamicWorker>(
        armonik::api::common::utils::Configuration(config), config, logger)
        ->run();
  } catch (const std::exception &e) {
    logger.fatal(std::string("Error in worker ") + e.what());
  }

  logger.info("Stopping ArmoniK SDK worker");
  return 0;
}
