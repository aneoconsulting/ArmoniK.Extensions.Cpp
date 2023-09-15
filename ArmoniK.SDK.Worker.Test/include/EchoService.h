#pragma once

#include <armonik/sdk/worker/ServiceBase.h>
#include <iostream>
namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

/**
 * @brief Example implementation of a ArmoniK::Sdk::Worker::ServiceBase
 */

/**
 * @brief Example implementation of a ArmoniK::Sdk::Worker::ServiceBase
 */
class EchoService : ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::string &input) override {
    std::cout << "EchoService method : " << name << std::endl;
    return input;
  }

  void *enter_session(const char *session_id) override {
    std::cout << "EchoService entering session : " << session_id << std::endl;
    return new std::string(session_id);
  }
  void leave_session(void *session_ctx) override {
    auto session_id = static_cast<std::string *>(session_ctx);
    std::cout << "EchoService leaving session : " << *session_id << std::endl;
    delete session_id;
  }
  ~EchoService() override { std::cout << "Deleted EchoService" << std::endl; };

  EchoService() : ServiceBase() { std::cout << "Created EchoService" << std::endl; }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
