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
class SegFaultService : ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::string &input) override {
    std::cout << "SegFaultService method : " << name << std::endl;
    *(volatile int *)0 = 0;
    return input;
  }

  void *enter_session(const char *session_id) override {
    std::cout << "SegFaultService entering session : " << session_id << std::endl;
    return new std::string(session_id);
  }
  void leave_session(void *session_ctx) override {
    auto session_id = static_cast<std::string *>(session_ctx);
    std::cout << "SegFaultService leaving session : " << *session_id << std::endl;
    delete session_id;
  }
  ~SegFaultService() override { std::cout << "Deleted SegFaultService" << std::endl; };

  SegFaultService() : ServiceBase() { std::cout << "Created SegFaultService" << std::endl; }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
