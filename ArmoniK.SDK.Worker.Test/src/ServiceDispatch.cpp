#include <ArmoniKSDKInterface.h>
#include <cstring>
#include <iostream>

#include "AdditionService.h"
#include "EchoService.h"

extern "C" void *armonik_create_service(const char *service_namespace, const char *service_name) {
  std::cout << "Creating service < " << service_namespace << "::" << service_name << " >" << std::endl;
  if (std::strcmp(service_name, "AdditionService") == 0) {
    return new SDK_END2END_NAMESPACE::AdditionService();
  } else if (std::strcmp(service_name, "EchoService") == 0) {
    return new SDK_END2END_NAMESPACE::EchoService();
  }

  std::cout << "Unknown service < " << service_namespace << "::" << service_name << " >" << std::endl;
  throw std::runtime_error(std::string("Unknown service <") + service_namespace + "::" + service_name + ">");
}
