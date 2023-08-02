#include "AdditionService.h"
#include "EchoService.h"
#include "ServiceBase.h"
#include <armonik/sdk/worker/ArmoniKSDKInterface.h>
#include <cstring>
#include <iostream>

/**
 * @brief
 *
 * @param name
 * @return void*
 */
extern "C" void *armonik_create_service_default(const char *service_namespace, const char *service_name) {
  std::cout << "Creating service < " << service_namespace << "::" << service_name << " >" << std::endl;
  if (std::strcmp(service_name, "AdditionService") == 0) {
    return new End2EndTest::AdditionService();
  } else if (std::strcmp(service_name, "EchoService") == 0) {
    return new End2EndTest::EchoService();
  }

  std::cout << "Unknown service < " << service_namespace << "::" << service_name << " >" << std::endl;
  throw std::runtime_error(std::string("Unknown service <") + service_namespace + "::" + service_name + ">");
}

__attribute__((weak, alias("armonik_create_service_default"))) void *
armonik_create_service(const char *service_namespace, const char *service_name);

extern "C" void armonik_destroy_service_default(void *p) {
  if (p) {
    delete static_cast<ServiceBase *>(p);
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
extern "C" void *armonik_enter_session_default(void *service_context, const char *session_id) {
  return static_cast<ServiceBase *>(service_context)->enter_session(session_id);
}

__attribute__((weak, alias("armonik_enter_session_default"))) void *armonik_enter_session(void *service_context,
                                                                                          const char *session_id);

/**
 * @brief
 *
 * @param service_context
 * @param session_context
 */
extern "C" void armonik_leave_session_default(void *service_context, void *session_context) {
  static_cast<ServiceBase *>(service_context)->leave_session(session_context);
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
extern "C" armonik_status_t armonik_call_default(void *armonik_context, void *service_context, void *session_context,
                                                 const char *function_name, const char *input, size_t input_size,
                                                 armonik_callback_t callback) {
  try {
    auto output = static_cast<ServiceBase *>(service_context)
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
