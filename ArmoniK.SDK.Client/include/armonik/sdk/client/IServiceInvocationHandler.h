#pragma once

#include <stdexcept>
#include <string>

#include <armonik/common/utils/string_view.h>

namespace ArmoniK {
namespace Sdk {
namespace Client {

/**
 * @brief Task result handler interface class
 */
class IServiceInvocationHandler {
public:
  /**
   * @brief Callback function called for each output of a task that succeeds.
   * @param result_payload Output data
   * @param taskId Task Id
   * @param result_id Blob ID of the output in ArmoniK storage; pass to BlobDefinition::FromBlobId
   *        to use this output as an input for a subsequent task without re-uploading.
   * @param output_name Name of the output, as declared in TaskDefinition::outputs
   *        (TaskDefinition::DefaultOutputName when the task declared none)
   * @note  Called concurrently for multiple outputs; implementation must be thread-safe.
   *
   * Override this version to handle tasks with several outputs. The default delegates to the
   * three-parameter overload.
   */
  virtual void HandleResponse(std::string &&result_payload, armonik::api::string_view taskId,
                              armonik::api::string_view result_id, armonik::api::string_view output_name) {
    (void)output_name;
    HandleResponse(std::move(result_payload), taskId, result_id);
  }

  /**
   * @brief Callback function called when a task succeeds.
   * @param result_payload Task result
   * @param taskId Task Id
   * @param result_id Blob ID of the result in ArmoniK storage; pass to BlobDefinition::FromBlobId
   *        to use this result as an input for a subsequent task without re-uploading.
   * @note  Called concurrently for multiple tasks; implementation must be thread-safe.
   */
  virtual void HandleResponse(std::string &&result_payload, armonik::api::string_view taskId,
                              armonik::api::string_view result_id) = 0;

  /**
   * @brief Callback function called when a tasks fails
   * @param e Risen error
   * @param taskId Task Id
   * @note  It can be called for multiple tasks in parallel, so it must be thread-safe
   */
  virtual void HandleError(const std::exception &e, armonik::api::string_view taskId) = 0;

  /**
   * @brief Callback function called for each output of a task that fails
   * @param e Risen error
   * @param taskId Task Id
   * @param output_name Name of the output that could not be retrieved
   * @note  A failed task with several outputs triggers one call per output. The default delegates to the
   *        two-parameter overload.
   */
  virtual void HandleError(const std::exception &e, armonik::api::string_view taskId,
                           armonik::api::string_view output_name) {
    (void)output_name;
    HandleError(e, taskId);
  }
};
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
