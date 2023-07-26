#include "workerlib/ComputeService.h"
#include <armonik/sdk/common/IConfiguration.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>
#include <armonik/worker/utils/WorkerServer.h>

int main() {
  std::cout << "Starting ArmoniK SDK worker" << std::endl;

  std::shared_ptr<ArmoniK::SDK::Common::IConfiguration> config =
      std::make_shared<ArmoniK::SDK::Common::IConfiguration>();

  config->set("ComputePlane__WorkerChannel__Address", "/cache/armonik_worker.sock");
  config->set("ComputePlane__AgentChannel__Address", "/cache/armonik_agent.sock");

  try {
    ArmoniK::Api::Worker::WorkerServer::create<ArmoniK::Api::Worker::ArmoniKWorker>(config, &computer)->run();
  } catch (const std::exception &e) {
    std::cout << "Error in worker" << e.what() << std::endl;
  }

  std::cout << "Stopping ArmoniK SDK worker" << std::endl;
  return 0;
}