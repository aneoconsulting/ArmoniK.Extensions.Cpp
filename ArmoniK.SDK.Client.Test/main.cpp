#include <iostream>

#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/IConfiguration.h>
#include <armonik/sdk/client/SessionService.h>

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
  std::cout << service.getSession() << std::endl;
  return 0;
}