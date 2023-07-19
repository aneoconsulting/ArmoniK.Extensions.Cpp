#include <iostream>

#include <armonik/sdk/client/IServiceInvocationHandler.h>
#include <armonik/sdk/client/SessionService.h>
#include <armonik/sdk/common/TaskRequest.h>
#include <armonik/sdk/common/IConfiguration.h>
#include <armonik/sdk/common/Properties.h>

class Handler : public ArmoniK::SDK::Client::IServiceInvocationHandler{
public:
  void HandleResponse(const std::string &result_payload, const std::string &taskId) override {

  }
  void HandleError(const std::exception &e, const std::string &taskId) override {

  }
};

int main() {
  std::cout << "Hello, World!" << std::endl;
  // Load configuration from file
  ArmoniK::SDK::Common::IConfiguration config;
  config.add_json_configuration("appsettings.json");
  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;
  // Create the properties
  ArmoniK::SDK::Common::Properties properties(
      config, ArmoniK::SDK::Common::TaskOptions("appName", "appVersion", "appNamespace", "appService"));
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
  auto handler = std::make_shared<Handler>();
  auto tasks = service.Submit({ArmoniK::SDK::Common::TaskRequest("TestService", "TestMethod", args, {})},handler);

  std::cout << "Sent : " << tasks[0] << std::endl;
  return 0;
}