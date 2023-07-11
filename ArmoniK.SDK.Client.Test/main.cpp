#include "SessionService.h"
#include <armonik/common/utils/JsonConfiguration.h>
#include <fstream>
#include <grpc++/grpc++.h>
#include <iostream>

int main() {
  std::cout << "Hello, World!" << std::endl;
  armonik::api::common::utils::JsonConfiguration config("appsettings.json");
  std::cout << "Endpoint : " << config.get("Grpc__EndPoint") << std::endl;
  auto channel = grpc::CreateChannel(config.get("Grpc__EndPoint"), grpc::InsecureChannelCredentials());
  ArmoniK::SDK::Client::SessionService service(channel);
  std::cout << service.getSession() << std::endl;
  return 0;
}