#ifndef ARMONIKSDKINTERFACE_H
#define ARMONIKSDKINTERFACE_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

/**
 * Low level interface
 */

#ifdef __cplusplus
extern "C" {
#endif
typedef enum armonik_status_t { ARMONIK_STATUS_OK = 0, ARMONIK_STATUS_ERROR = 1 } armonik_status_t;

/**
 * @brief
 *
 * @param service_namespace
 * @param service_name
 * @return void*
 */
extern void *armonik_create_service(const char *service_namespace, const char *service_name);
typedef void *(*armonik_create_service_t)(const char *service_namespace, const char *service_name);

/**
 * @brief
 *
 * @param service_context
 */
void armonik_destroy_service(void *service_context);
typedef void (*armonik_destroy_service_t)(void *service_context);

/**
 * @brief
 *
 * @param service_context
 * @param session_id
 * @return void*
 */
void *armonik_enter_session(void *service_context, const char *session_id);
typedef void *(*armonik_enter_session_t)(void *service_context, const char *session_id);

/**
 * @brief
 *
 * @param service_context
 * @param session_context
 */
void armonik_leave_session(void *service_context, void *session_context);
typedef void (*armonik_leave_session_t)(void *service_context, void *session_context);

typedef void (*armonik_callback_t)(void *armonik_context, armonik_status_t status, const char *output_or_error,
                                   size_t output_size);

/**
 * @brief
 *
 * @param armonik_context
 * @param service_context
 * @param session_context
 * @param function_name
 * @param input
 * @param input_size
 * @param callback
 */
armonik_status_t armonik_call(void *armonik_context, void *service_context, void *session_context,
                              const char *function_name, const char *input, size_t input_size,
                              armonik_callback_t callback);

typedef armonik_status_t (*armonik_call_t)(void *armonik_context, void *service_context, void *session_context,
                                           const char *function_name, const char *input, size_t input_size,
                                           armonik_callback_t callback);
#ifdef __cplusplus
}
#endif

#endif
