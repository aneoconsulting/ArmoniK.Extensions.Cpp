#include <armonik/sdk/common/IConfiguration.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <armonik/worker/Worker/ArmoniKWorker.h>
#include <armonik/worker/Worker/ProcessStatus.h>
#include <armonik/worker/Worker/TaskHandler.h>
#include <armonik/worker/utils/WorkerServer.h>
#include <armonik/common/exceptions/ArmoniKApiException.h>

class Computer final : public ArmoniK::Api::Worker::ArmoniKWorker {
public:
  explicit Computer(std::unique_ptr<armonik::api::grpc::v1::agent::Agent::Stub> agent)
      : ArmoniKWorker(std::move(agent)) {}

  ArmoniK::Api::Worker::ProcessStatus Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler) override {
    std::cout << "Call computer" << std::endl;
    std::cout << "SizePayload : " << taskHandler.getPayload().size()
              << "\nSize DD : " << taskHandler.getDataDependencies().size()
              << "\n Expected results : " << taskHandler.getExpectedResults().size() << std::endl;

    try {
      if (!taskHandler.getExpectedResults().empty()) {
        auto res = taskHandler.send_result(taskHandler.getExpectedResults()[0], taskHandler.getPayload()).get();
        if (res.has_error()) {
          throw ArmoniK::Api::Common::exceptions::ArmoniKApiException(res.error());
        }
      }

    } catch (const std::exception &e) {
      std::cout << "Error sending result " << e.what() << std::endl;
      return ArmoniK::Api::Worker::ProcessStatus(e.what());
    }

    return ArmoniK::Api::Worker::ProcessStatus::OK;
  }

};

int main() {
  std::cout << "Starting ArmoniK SDK worker" << std::endl;

  std::shared_ptr<ArmoniK::Api::Common::utils::IConfiguration> config =
  std::make_shared<ArmoniK::Api::Common::utils::IConfiguration>();

  config->set("ComputePlane__WorkerChannel__Address", "/cache/armonik_worker.sock");
  config->set("ComputePlane__AgentChannel__Address", "/cache/armonik_agent.sock");

  try {
    ArmoniK::Api::Worker::WorkerServer::create<Computer>(config)->run();
  } catch (const std::exception &e) {
    std::cout << "Error in worker" << e.what() << std::endl;
  }

  std::cout << "Stopping ArmoniK SDK worker" << std::endl;
  return 0;
}