#pragma once

#include <armonik/sdk/worker/ServiceBase.h>
#include <map>
#include <stdexcept>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

class ConventionService : ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::map<std::string, std::string> &inputs) override {
    if (name == "square") {
      int x = std::stoi(inputs.at("x"));
      return std::to_string(x * x);
    }
    if (name == "add") {
      int a = std::stoi(inputs.at("a"));
      int b = std::stoi(inputs.at("b"));
      return std::to_string(a + b);
    }
    throw std::runtime_error("ConventionService: unknown method: " + name);
  }

  void *enter_session(const char *session_id) override { return new std::string(session_id); }
  void leave_session(void *session_ctx) override { delete static_cast<std::string *>(session_ctx); }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
