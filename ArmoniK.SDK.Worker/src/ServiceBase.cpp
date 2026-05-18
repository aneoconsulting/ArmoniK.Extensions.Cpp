#include "armonik/sdk/worker/ServiceBase.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include <nlohmann/json.hpp>

namespace ArmoniK {
namespace Sdk {
namespace Worker {

std::string ServiceBase::call(void *session_ctx, const std::string &name, const std::string &input) {
  try {
    auto inputs = nlohmann::json::parse(input).at("inputs").get<std::map<std::string, std::string>>();
    return call(session_ctx, name, inputs);
  } catch (const nlohmann::json::exception &e) {
    throw Common::ArmoniKSdkException(std::string("Failed to parse convention payload: ") + e.what());
  }
}

std::string ServiceBase::call(void *session_ctx, const std::string &name,
                              const std::map<std::string, std::string> &inputs) {
  (void)session_ctx;
  (void)inputs;
  throw Common::ArmoniKSdkException("ServiceBase::call not implemented for method: " + name);
}

} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
