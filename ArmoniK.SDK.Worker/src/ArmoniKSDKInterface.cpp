#include "ServiceBase.h"
#include <ArmoniKSDKInterface.h>
#include <cstring>

extern "C" {

void armonik_destroy_service_default(void *p) {
  if (p) {
    delete static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(p);
  }
}

/**
 * @brief
 *
 * @param service_context
 */
__attribute__((weak, alias("armonik_destroy_service_default"))) void armonik_destroy_service(void *p);

/**
 * @brief
 *
 * @param service_context
 * @param session_id
 * @return void*
 */
void *armonik_enter_session_default(void *service_context, const char *session_id) {
  return static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(service_context)->enter_session(session_id);
}

__attribute__((weak, alias("armonik_enter_session_default"))) void *armonik_enter_session(void *service_context,
                                                                                          const char *session_id);

/**
 * @brief
 *
 * @param service_context
 * @param session_context
 */
void armonik_leave_session_default(void *service_context, void *session_context) {
  static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(service_context)->leave_session(session_context);
}

__attribute__((weak, alias("armonik_leave_session_default"))) void armonik_leave_session(void *service_context,
                                                                                         void *session_context);

/**
 * @brief
 *
 * @param service_context
 * @param session_context
 * @param function_name
 * @param input
 * @param input_size
 * @param callback
 */
armonik_status_t armonik_call_default(void *armonik_context, void *service_context, void *session_context,
                                      const char *function_name, const char *input, size_t input_size,
                                      armonik_callback_t callback) {
  try {
    auto output = static_cast<ArmoniK::Sdk::Worker::ServiceBase *>(service_context)
                      ->call(session_context, std::string(function_name), std::string(input, input_size));
    callback(armonik_context, ARMONIK_STATUS_OK, output.data(), output.size());
    return ARMONIK_STATUS_OK;
  } catch (const std::exception &e) {
    auto msg = e.what();
    callback(armonik_context, ARMONIK_STATUS_ERROR, msg, std::strlen(msg));
    return ARMONIK_STATUS_ERROR;
  }
}

__attribute__((weak, alias("armonik_call_default"))) armonik_status_t
armonik_call(void *armonik_context, void *service_context, void *session_context, const char *function_name,
             const char *input, size_t input_size, armonik_callback_t callback);
}