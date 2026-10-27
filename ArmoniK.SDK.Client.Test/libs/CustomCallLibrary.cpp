// Worker library that links ArmoniK.SDK.Worker but defines its own armonik_call, with a service context that is not
// a ServiceBase. The default armonik_process of ArmoniK.SDK.Worker must send its tasks through this armonik_call.

#include <armonik/sdk/worker/ArmoniKSDKInterface.h>
#include <string>

namespace {
struct CustomService {
  std::string prefix = "custom:";
};
} // namespace

extern "C" {

void *armonik_create_service(const char *, const char *) { return new CustomService(); }

void armonik_destroy_service(void *service_context) { delete static_cast<CustomService *>(service_context); }

void *armonik_enter_session(void *, const char *) { return nullptr; }

void armonik_leave_session(void *, void *) {}

armonik_status_t armonik_call(void *armonik_context, void *service_context, void *, const char *function_name,
                              const char *input, size_t input_size, armonik_callback_t callback) {
  const std::string name(function_name);
  if (name == "fail") {
    const std::string message = "custom failure";
    callback(armonik_context, ARMONIK_STATUS_ERROR, message.data(), message.size());
    return ARMONIK_STATUS_ERROR;
  }
  const auto output = static_cast<CustomService *>(service_context)->prefix + std::string(input, input_size);
  callback(armonik_context, ARMONIK_STATUS_OK, output.data(), output.size());
  return ARMONIK_STATUS_OK;
}
}
