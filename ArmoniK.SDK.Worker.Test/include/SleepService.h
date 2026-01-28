#pragma once

#include <armonik/sdk/worker/ServiceBase.h>
#include <chrono>
#include <iostream>
#include <thread>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

/**
 * @brief Example implementation of a ArmoniK::Sdk::Worker::ServiceBase
 */
class SleepService : ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::string &input) override {
    std::cout << "SleepService method: " << name << std::endl;
    try {
      int sleepTimeMs = std::stoi(input);
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
    } catch (const std::invalid_argument &) {
      std::cerr << "Invalid input: not a number. Sleeping for default 1000 ms." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return input;
  }
  void *enter_session(const char *session_id) override {
    std::cout << "SleepService entering session : " << session_id << std::endl;
    return new std::string(session_id);
  }
  void leave_session(void *session_ctx) override {
    auto session_id = static_cast<std::string *>(session_ctx);
    std::cout << "SleepService leaving session : " << *session_id << std::endl;
    delete session_id;
  }
  ~SleepService() override { std::cout << "Deleted SleepService" << std::endl; };

  SleepService() : ServiceBase() { std::cout << "Created SleepService" << std::endl; }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
