#include <iostream>

#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <armonik/sdk/client/SessionService.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/common/Version.h>

/**
 * Opens a session on a port with no server behind it, the call is expected to fail.
 */
int main() {
  std::cout << "ArmoniK SDK " << ArmoniK::Sdk::Common::getVersion() << std::endl;

  armonik::api::common::logger::Logger logger{armonik::api::common::logger::writer_console(),
                                              armonik::api::common::logger::formatter_plain(true)};
  ArmoniK::Sdk::Common::Configuration config;
  config.set("GrpcClient__Endpoint", "http://127.0.0.1:1");
  ArmoniK::Sdk::Common::TaskOptions task_options("smoke", "1.0.0", "smoke", "smoke");
  ArmoniK::Sdk::Common::Properties properties{config, task_options};

  try {
    ArmoniK::Sdk::Client::SessionService service(properties, logger);
  } catch (const std::exception &e) {
    std::cout << "Expected failure: " << e.what() << std::endl;
    return 0;
  }
  std::cerr << "Session creation unexpectedly succeeded" << std::endl;
  return 1;
}
