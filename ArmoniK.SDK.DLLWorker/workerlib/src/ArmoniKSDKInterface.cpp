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
#ifdef __linux__
__attribute__((weak))
#endif
void *
armonik_create_service(const char *service_namespace, const char *service_name) {
  std::cout << "Creating service < " << service_namespace << "::" << service_name << " >" << std::endl;
  if (std::strcmp(service_name, "AdditionService") == 0) {
    return new End2EndTest::AdditionService();
  } else if (std::strcmp(service_name, "EchoService") == 0) {
    return new End2EndTest::EchoService();
  }

  std::cout << "Unknown service < " << service_namespace << "::" << service_name << " >" << std::endl;
  throw std::runtime_error(std::string("Unknown service <") + service_namespace + "::" + service_name + ">");
}

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
#ifdef __linux__
__attribute__((weak, alias("armonik_destroy_service_default"))) void armonik_destroy_service(void *p);
#endif

/**
 * @brief
 *
 * @param service_context
 * @param session_id
 * @return void*
 */
#ifdef __linux__
__attribute__((weak))
#endif
void *
armonik_enter_session(void *service_context, const char *session_id) {
  return static_cast<ServiceBase *>(service_context)->enter_session(session_id);
}

/**
 * @brief
 *
 * @param service_context
 * @param session_context
 */
#ifdef __linux__
__attribute__((weak))
#endif
void armonik_leave_session(void* service_context, void* session_context) {
  static_cast<ServiceBase *>(service_context)->leave_session(session_context);
}

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
#ifdef __linux__
__attribute__((weak))
#endif
armonik_status_t
armonik_call(void *armonik_context, void *service_context, void *session_context, const char *function_name,
             const char *input, size_t input_size, armonik_callback_t callback) {
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
