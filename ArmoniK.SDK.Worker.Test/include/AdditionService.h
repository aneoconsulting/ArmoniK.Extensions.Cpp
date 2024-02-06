#pragma once

#include <armonik/sdk/worker/ServiceBase.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

/**
 * \example AdditionService.h
 * Example implementation of a ArmoniK::Sdk::Worker::ServiceBase
 */

/**
 * @brief Example implementation of a ArmoniK::Sdk::Worker::ServiceBase
 */
class AdditionService : ServiceBase {
public:
  void *enter_session(const char *session_id) override {
    std::cout << "AdditionService enter session" << *session_id << std::endl;
    return new std::string(session_id);
  }
  void leave_session(void *session_ctx) override {
    auto sessionId = static_cast<std::string *>(session_ctx);
    std::cout << "AdditionService leave session" << *sessionId << std::endl;
    delete sessionId;
  }
  std::string call(void *, const std::string &name, const std::string &input) override {
    if (name == "add_ints") {
      char output[sizeof(int32_t)];
      int32_t a, b, c;
      std::memcpy(&a, input.data(), sizeof(int32_t));
      std::memcpy(&b, input.data() + sizeof(int32_t), sizeof(int32_t));
      c = add_ints(a, b);
      std::memcpy(output, &c, sizeof(int32_t));
      return {output, sizeof(int32_t)};
    } else if (name == "add_floats") {
      char output[sizeof(float)];
      float a, b, c;
      std::memcpy(&a, input.data(), sizeof(float));
      std::memcpy(&b, input.data() + sizeof(float), sizeof(float));
      c = add_floats(a, b);
      std::memcpy(output, &c, sizeof(float));
      return {output, sizeof(float)};
    }
    throw std::runtime_error("Unknown method name" + name);
  }

  /**
   * @brief Adds 2 ints
   * @param a A
   * @param b B
   * @return A+B
   */
  int32_t add_ints(int32_t a, int32_t b) {
    (void)this;
    return a + b;
  }

  /**
   * @brief Add 2 floats
   * @param a A
   * @param b B
   * @return A+B
   */
  float add_floats(float a, float b) {
    (void)this;
    return a + b;
  }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
