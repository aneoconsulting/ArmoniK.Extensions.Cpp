#ifndef ARMONIK_SDK_ADDITIONSERVICE_H
#define ARMONIK_SDK_ADDITIONSERVICE_H

#include <ServiceBase.h>
#include <iostream>
#include <stdexcept>
namespace SDK_END2END_NAMESPACE {

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
  std::string call(void *session_ctx, const std::string &name, const std::string &input) override {
    (void)session_ctx;
    std::string output;
    if (name == "add_ints") {
      output.resize(sizeof(int32_t));
      int32_t a, b, c;
      std::memcpy(&a, input.data(), sizeof(int32_t));
      std::memcpy(&b, input.data() + sizeof(int32_t), sizeof(int32_t));
      c = add_ints(a, b);
      std::memcpy(output.data(), &c, sizeof(int32_t));
      return output;
    } else if (name == "add_floats") {
      output.resize(sizeof(float));
      float a, b, c;
      std::memcpy(&a, input.data(), sizeof(float));
      std::memcpy(&b, input.data() + sizeof(float), sizeof(float));
      c = add_floats(a, b);
      std::memcpy(output.data(), &c, sizeof(float));
      return output;
    }
    throw std::runtime_error("Unknown method name" + name);
  }
  int32_t add_ints(int32_t a, int32_t b) {
    (void)this;
    return a + b;
  }
  float add_floats(float a, float b) {
    (void)this;
    return a + b;
  }
};

} // namespace SDK_END2END_NAMESPACE

#endif // ARMONIK_SDK_ADDITIONSERVICE_H
