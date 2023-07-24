#include <armonik/sdk/common/IConfiguration.h>
#include <armonik/sdk/worker/BaseService.h>
#include <armonik/sdk/worker/ComputeService.h>
#include <armonik/sdk/common/TaskRequest.h>
#include <armonik/sdk/common/TaskOptions.h>

#include <armonik/worker/Worker/TaskHandler.h>
#include <armonik/worker/Worker/ProcessStatus.h>

using ArmoniK::Api::Common::utils::IConfiguration;
using armonik::api::grpc::v1::TaskOptions;

using namespace armonik::api::grpc::v1::worker;
using namespace ArmoniK::Api::Common::utils;

int main() {
  std::cout << "Starting ArmoniK SDK worker" << std::endl;

  std::shared_ptr<IConfiguration> config = std::make_shared<IConfiguration>();

  config->set("ComputePlane__WorkerChannel__Address", "/cache/armonik_worker.sock");
  config->set("ComputePlane__AgentChannel__Address", "/cache/armonik_agent.sock");

  std::cout << "Stopping ArmoniK SDK worker" << std::endl;
  return 0;
}