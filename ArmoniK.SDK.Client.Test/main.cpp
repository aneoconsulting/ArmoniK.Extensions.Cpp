#include <iostream>

#include <armonik/sdk/client/IServiceInvocationHandler.h>
#include <armonik/sdk/client/SessionService.h>
#include <armonik/sdk/common/IConfiguration.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskPayload.h>

class PythonTestWorkerHandler : public ArmoniK::SDK::Client::IServiceInvocationHandler {
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override {
    std::cout << "HANDLE RESPONSE : Received result of size " << result_payload.size() << std::endl;
    auto tr = ArmoniK::SDK::Common::TaskPayload::Deserialize(result_payload);
    std::cout << "Received : "
              << "\n Method name : " << tr.method_name << "\n Data dependencies : \n";
    for (auto &&dd : tr.data_dependencies) {
      std::cout << " - " << dd << '\n';
    }
    std::cout << " Args length : " << tr.arguments.size() << std::endl;
  }
  void HandleError(const std::exception &e, const std::string &taskId) override {
    std::cerr << "HANDLE ERROR : Error for task id " << taskId << " : " << e.what() << std::endl;
  }
};

int main() {
  std::cout << "Hello, World!" << std::endl;
  // Load configuration from file and environment
  ArmoniK::SDK::Common::IConfiguration config;
  config.add_json_configuration("appsettings.json").add_env_configuration();

  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;

  // Create the task options
  ArmoniK::SDK::Common::TaskOptions session_task_options("appName", "appVersion", "appNamespace", "appService");

  // Create the properties
  ArmoniK::SDK::Common::Properties properties(config, session_task_options);

  // Create the session service
  ArmoniK::SDK::Client::SessionService service(properties);

  // Get the created session id
  std::cout << "Session : " << service.getSession() << std::endl;
  std::string args;
  args.resize(10);
  args[0] = 'A';
  args[1] = 'r';
  args[2] = 'm';
  args[3] = '0';
  args[4] = '\0';
  args[5] = 'n';
  args[6] = 1;
  args[7] = 'K';
  args[8] = 0;
  args[9] = -128;

  // Create the handler
  auto handler = std::make_shared<PythonTestWorkerHandler>();

  // Submit a task
  auto tasks = service.Submit({ArmoniK::SDK::Common::TaskPayload("TestMethod", args)}, handler);

  std::cout << "Sent : " << tasks[0] << std::endl;

  // Wait for task completion
  service.WaitResults();

  std::cout << "Done" << std::endl;
  return 0;
}