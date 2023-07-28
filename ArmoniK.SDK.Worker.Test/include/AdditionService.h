#ifndef ARMONIK_SDK_ADDITIONSERVICE_H
#define ARMONIK_SDK_ADDITIONSERVICE_H

#include "ServiceBase.h"
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
  std::string call(void *session_ctx, const std::string &name, const std::string& input) override {
    std::string output;
    output.resize(sizeof(int32_t));
    if (name == "add_ints") {
      *reinterpret_cast<int32_t *>(output.data()) =
          add_ints(*reinterpret_cast<const int32_t *>(input.data()),
                   *reinterpret_cast<const int32_t *>(input.data() + sizeof(int32_t)));
      return output;
    } else if (name == "add_floats") {
      *reinterpret_cast<float *>(output.data()) =
          add_floats(*reinterpret_cast<const float *>(input.data()),
                     *reinterpret_cast<const float *>(input.data() + sizeof(float)));
      return output;
    }
    throw std::runtime_error("Unknown method name" + name);
  }
  int32_t add_ints(int32_t a, int32_t b) { return a + b; }
  float add_floats(float a, float b) { return a + b; }
};

} // namespace SDK_END2END_NAMESPACE

#endif // ARMONIK_SDK_ADDITIONSERVICE_H
