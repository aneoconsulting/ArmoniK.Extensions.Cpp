#include <armonik/sdk/worker/ArmoniKSDKInterface.h>
#include <cstring>
#include <iostream>

#include "AdditionService.h"
#include "EchoService.h"

extern "C" void *armonik_create_service(const char *service_namespace, const char *service_name) {
  std::cout << "Creating service < " << service_namespace << "::" << service_name << " >" << std::endl;
  if (std::strcmp(service_name, "AdditionService") == 0) {
    return new ArmoniK::Sdk::Worker::Test::AdditionService();
  } else if (std::strcmp(service_name, "EchoService") == 0) {
    return new ArmoniK::Sdk::Worker::Test::EchoService();
  }

  std::cout << "Unknown service < " << service_namespace << "::" << service_name << " >" << std::endl;
  throw std::runtime_error(std::string("Unknown service <") + service_namespace + "::" + service_name + ">");
}

/** \example ServiceDispatch.cpp
 * This is an example on how to use the ArmoniK::Sdk::Worker::ServiceBase approach for the ArmoniK Worker
 * See ArmoniK::Sdk::Worker::Test::AdditionService and ArmoniK::Sdk::Worker::Test::EchoService for example
 * implementations
 */
