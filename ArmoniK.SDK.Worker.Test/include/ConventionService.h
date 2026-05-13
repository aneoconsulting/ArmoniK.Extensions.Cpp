#pragma once

#include <armonik/sdk/worker/ServiceBase.h>
#include <stdexcept>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Test {

class ConventionService : ServiceBase {
public:
  std::string call(void *, const std::string &name, const std::string &input) override {
    if (name == "square") {
      int x = std::stoi(extract_input(input, "x"));
      return std::to_string(x * x);
    }
    if (name == "add") {
      int a = std::stoi(extract_input(input, "a"));
      int b = std::stoi(extract_input(input, "b"));
      return std::to_string(a + b);
    }
    throw std::runtime_error("ConventionService: unknown method: " + name);
  }

  void *enter_session(const char *session_id) override { return new std::string(session_id); }
  void leave_session(void *session_ctx) override { delete static_cast<std::string *>(session_ctx); }

private:
  // Extract the string value for `key` from the "inputs" object of a TaskPayload JSON.
  // Relies on the known wire format: {"inputs":{"key":"value",...},...}
  static std::string extract_input(const std::string &json, const std::string &key) {
    const std::string needle = "\"" + key + "\":\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos)
      throw std::runtime_error("ConventionService: input key not found: " + key);
    pos += needle.size();
    const auto end = json.find('"', pos);
    if (end == std::string::npos)
      throw std::runtime_error("ConventionService: malformed payload for key: " + key);
    return json.substr(pos, end - pos);
  }
};

} // namespace Test
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
