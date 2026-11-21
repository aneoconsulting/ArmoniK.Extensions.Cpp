#include "armonik/sdk/worker/ServiceBase.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include "armonik/sdk/common/internal/ConventionPayload.h"
#include "armonik/sdk/worker/TaskContext.h"
#include <nlohmann/json.hpp>

namespace ArmoniK {
namespace Sdk {
namespace Worker {

namespace {
/**
 * @brief Thrown by the default of the map overload of call, so that process tells it from a failure of the service
 */
class CallNotImplemented final : public Common::ArmoniKSdkException {
public:
  using Common::ArmoniKSdkException::ArmoniKSdkException;
};
} // namespace

void ServiceBase::process(void *session_ctx, const std::string &name, TaskContext &context) {
  auto &outputs = context.outputs();
  if (outputs.size() != 1) {
    throw Common::ArmoniKSdkException("Method " + name + " has " + std::to_string(outputs.size()) +
                                      " outputs: override ServiceBase::process to produce them");
  }
  auto &output = outputs.begin()->second;
  if (context.default_call() == TaskContext::DefaultCall::Inputs) {
    output.write(call(session_ctx, name, context.input_values()));
    return;
  }
  // Payload overload: services that override it receive the payload with the input values, as armonik_call gave it
  Common::ConventionPayload payload;
  payload.inputs = context.input_values();
  payload.outputs.emplace(output.name(), output.id());
  std::string serialized;
  try {
    serialized = payload.Serialize();
  } catch (const Common::ArmoniKSdkException &e) {
    // JSON cannot hold an input that is not valid UTF-8. The default of the payload overload would only pass the
    // inputs to the map overload: call it directly, which fails only if the service overrides neither.
    try {
      output.write(call(session_ctx, name, context.input_values()));
      return;
    } catch (const CallNotImplemented &) {
      throw Common::ArmoniKSdkException(std::string(e.what()) +
                                        ". An input is not valid UTF-8, so it cannot reach call(session_ctx, name, "
                                        "payload): override call(session_ctx, name, inputs) or process instead");
    }
  }
  output.write(call(session_ctx, name, serialized));
}

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
  throw CallNotImplemented("ServiceBase::call not implemented for method: " + name);
}

} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
