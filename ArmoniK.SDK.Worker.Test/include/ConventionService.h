#pragma once

#include <armonik/sdk/worker/ServiceBase.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

class ConventionService : ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::string &input) override {
    const auto inputs = nlohmann::json::parse(input).at("inputs");
    if (name == "square") {
      int x = std::stoi(inputs.at("x").get<std::string>());
      return std::to_string(x * x);
    }
    if (name == "add") {
      int a = std::stoi(inputs.at("a").get<std::string>());
      int b = std::stoi(inputs.at("b").get<std::string>());
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
