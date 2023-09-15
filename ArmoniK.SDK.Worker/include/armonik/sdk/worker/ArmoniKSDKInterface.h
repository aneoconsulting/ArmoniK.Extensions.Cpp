#pragma once

/**
 * \file ArmoniKSDKInterface.h
 * Low level interface to communicate between the DynamicWorker and the loaded library.
 *
 * This file can be used as-is to implement a DynamicWorker-compatible library. You may also use the ArmoniK.SDK.Worker
 * library to have a simple wrapper.
 */

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief ArmoniK call status
 * \anchor armonik_status_t
 */
typedef enum armonik_status_t {
  /**
   * @brief Successful call
   */
  ARMONIK_STATUS_OK = 0,
  /**
   * @brief Failed called
   * @note This status should be used only if the failure reason is not solvable by a retry. The task result will then
   * be marked as ABORTED.
   */
  ARMONIK_STATUS_ERROR = 1
} armonik_status_t;

/**
 * @brief Function called when requesting a service creation
 * @param service_namespace Namespace of the requested service, may be empty
 * @param service_name Name of the requested service
 * @return A user defined service context
 * @warning When using the ArmoniK.SDK.Worker library, this function is the only mandatory one to be implemented and \b
 * MUST return a pointer to a subclass of ArmoniK::Sdk::Worker::ServiceBase
 */
extern void *armonik_create_service(const char *service_namespace, const char *service_name);

/**
 * @brief armonik_create_service function typedef
 */
typedef void *(*armonik_create_service_t)(const char *service_namespace, const char *service_name);

/**
 * @brief Function called when requesting the destruction of a service
 * @param service_context User defined service context
 * @note When using the ArmoniK.SDK.Worker library, this function is already implemented and the service_context is a
 * ServiceBase. You can override this method to add functionality but you should also call
 * armonik_destroy_service_default to destroy the service_context.
 */
void armonik_destroy_service(void *service_context);

/**
 * @brief armonik_destroy_service function typedef
 */
typedef void (*armonik_destroy_service_t)(void *service_context);

/**
 * @brief Function called when entering a session
 * @param service_context User defined service context
 * @param session_id session id
 * @return User defined session context
 * @note When using the ArmoniK.SDK.Worker library, this function is already implemented and calls the enter_session
 * method of the service.
 */
void *armonik_enter_session(void *service_context, const char *session_id);

/**
 * @brief armonik_enter_session function typedef
 */
typedef void *(*armonik_enter_session_t)(void *service_context, const char *session_id);

/**
 * @brief Function called when leaving a session
 * @param service_context User defined service context
 * @param session_context User defined session context
 * @note When using the ArmoniK.SDK.Worker library, this function is already implemented and calls the leave_session
 * method of the service.
 */
void armonik_leave_session(void *service_context, void *session_context);

/**
 * @brief armonik_leave_session function typedef
 */
typedef void (*armonik_leave_session_t)(void *service_context, void *session_context);

/**
 * @brief armonik_callback function typedef
 */
typedef void (*armonik_callback_t)(void *armonik_context, armonik_status_t status, const char *output_or_error,
                                   size_t output_size);

/**
 * @brief Function called when requesting the execution of a method
 * @param armonik_context Opaque ArmoniK context, should be passed to the callback as-is without modification
 * @param service_context User defined service context
 * @param session_context User defined session context
 * @param function_name Name of the function to call
 * @param input Serialized arguments for the function
 * @param input_size size of the serialized arguments
 * @param callback Callback provided by ArmoniK to send the result of the execution or details on a failure
 * @return Status of the call. See /ref armonik_status_t for more details
 */
armonik_status_t armonik_call(void *armonik_context, void *service_context, void *session_context,
                              const char *function_name, const char *input, size_t input_size,
                              armonik_callback_t callback);

/**
 * @brief armonik_call function typedef
 */
typedef armonik_status_t (*armonik_call_t)(void *armonik_context, void *service_context, void *session_context,
                                           const char *function_name, const char *input, size_t input_size,
                                           armonik_callback_t callback);
#ifdef __cplusplus
}
#endif
