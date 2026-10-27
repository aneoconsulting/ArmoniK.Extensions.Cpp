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
   * @brief Failed call, not retryable.
   * @note Use this status for permanent failures. The task will be marked as Error and will not be retried.
   * @note When using the ArmoniK.SDK.Worker library, throwing an
   * ArmoniK::Sdk::Common::ArmoniKSdkException from ServiceBase::call() will automatically produce this status.
   */
  ARMONIK_STATUS_ERROR = 1,
  /**
   * @brief Failed call, retryable.
   * @note Use this status for transient failures. ArmoniK will retry the task according to the task's retry policy
   * (TaskOptions::max_retries).
   * @note When using the ArmoniK.SDK.Worker library, any exception other than
   * ArmoniK::Sdk::Common::ArmoniKSdkException thrown from ServiceBase::call() will automatically produce this status.
   */
  ARMONIK_STATUS_RETRY = 2
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

/**
 * @brief Bytes, not null-terminated
 */
typedef struct armonik_string_t {
  const char *data;
  size_t size;
} armonik_string_t;

/**
 * @brief Receives a string from the worker. The string is valid only during the call.
 */
typedef void (*armonik_string_sink_t)(void *sink_context, const char *data, size_t size);

/**
 * @brief An input of a convention task
 */
typedef struct armonik_input_t {
  armonik_string_t name;
  /** Blob id of the input, empty when the value was given inline in the payload */
  armonik_string_t id;
  armonik_string_t data;
} armonik_input_t;

/**
 * @brief An output of a convention task
 */
typedef struct armonik_output_t {
  armonik_string_t name;
  armonik_string_t id;
} armonik_output_t;

/**
 * @brief Data to write to a result
 */
typedef struct armonik_result_data_t {
  armonik_string_t id;
  armonik_string_t data;
} armonik_result_data_t;

/**
 * @brief Task to submit
 */
typedef struct armonik_task_request_t {
  armonik_string_t payload;
  const armonik_string_t *data_dependencies;
  size_t data_dependencies_count;
  const armonik_string_t *expected_outputs;
  size_t expected_outputs_count;
} armonik_task_request_t;

/**
 * @brief A convention task, and the operations it can do on ArmoniK, given by the worker to armonik_process.
 *
 * Each operation returns ARMONIK_STATUS_OK, or another status after calling on_error with a message.
 * ARMONIK_STATUS_RETRY means a transient failure. Strings given to the sinks are valid only during the call.
 * @note Task options are a JSON object with the fields of ArmoniK::Sdk::Common::TaskOptions.
 */
typedef struct armonik_task_t {
  /** sizeof(armonik_task_t) in the worker, so that fields can be added at the end */
  size_t struct_size;
  /** Opaque worker context, passed back to the operations */
  void *agent;
  armonik_string_t session_id;
  armonik_string_t task_id;
  /** Options of the task, as JSON */
  armonik_string_t task_options;
  const armonik_input_t *inputs;
  size_t inputs_count;
  const armonik_output_t *outputs;
  size_t outputs_count;

  /** Creates results without data. on_value receives their ids, in the order of names. */
  armonik_status_t (*create_results_metadata)(void *agent, const armonik_string_t *names, size_t count,
                                              void *sink_context, armonik_string_sink_t on_value,
                                              armonik_string_sink_t on_error);
  /** Writes the data of results that the task created or must produce */
  armonik_status_t (*send_results)(void *agent, const armonik_result_data_t *results, size_t count, void *sink_context,
                                   armonik_string_sink_t on_error);
  /** Submits tasks with the given options (JSON). on_value receives their ids, in the order of tasks. */
  armonik_status_t (*submit_tasks)(void *agent, armonik_string_t task_options, const armonik_task_request_t *tasks,
                                   size_t count, void *sink_context, armonik_string_sink_t on_value,
                                   armonik_string_sink_t on_error);
} armonik_task_t;

/**
 * @brief Function called to execute a convention task with its context. Optional: when a library does not export
 * it, the worker uses armonik_call, which supports only tasks with a single output.
 * @param armonik_context Opaque ArmoniK context, should be passed to the callback as-is without modification
 * @param task Task and its operations, valid only during the call
 * @param service_context User defined service context
 * @param session_context User defined session context
 * @param function_name Name of the function to call
 * @param callback Callback provided by ArmoniK, called only on a failure, with its message. The task writes its
 *        outputs through task->send_results.
 * @return Status of the call. See /ref armonik_status_t for more details
 * @note When using the ArmoniK.SDK.Worker library, this function is already implemented and calls
 * ServiceBase::process.
 */
armonik_status_t armonik_process(void *armonik_context, const armonik_task_t *task, void *service_context,
                                 void *session_context, const char *function_name, armonik_callback_t callback);

/**
 * @brief armonik_process function typedef
 */
typedef armonik_status_t (*armonik_process_t)(void *armonik_context, const armonik_task_t *task, void *service_context,
                                              void *session_context, const char *function_name,
                                              armonik_callback_t callback);
#ifdef __cplusplus
}
#endif
