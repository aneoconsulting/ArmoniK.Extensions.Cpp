#include "Properties.h"
#include "SessionService.h"
#include <armonik/common/utils/JsonConfiguration.h>
#include <fstream>
#include <grpc++/grpc++.h>
#include <iostream>

int main() {
  std::cout << "Hello, World!" << std::endl;
  armonik::api::common::utils::JsonConfiguration config("appsettings.json");
  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;
  ArmoniK::SDK::Common::Properties properties(
      config, ArmoniK::SDK::Common::TaskOptions("appName", "appVersion", "appNamespace", "appService"));
  ArmoniK::SDK::Client::SessionService service(properties);
  std::cout << service.getSession() << std::endl;
  return 0;
}