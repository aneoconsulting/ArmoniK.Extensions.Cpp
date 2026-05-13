#pragma once

#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Client {

/**
 * @brief Task result handler interface class
 */
class IServiceInvocationHandler {
public:
  /**
   * @brief Callback function called when a task succeeds.
   * @param result_payload Task result
   * @param taskId Task Id
   * @param result_id Blob ID of the result in ArmoniK storage; pass to BlobDefinition::FromBlobId
   *        to use this result as an input for a subsequent task without re-uploading.
   * @note  Called concurrently for multiple tasks; implementation must be thread-safe.
   *
   * Override this version to access result_id. The default delegates to the two-parameter
   * overload below for backward compatibility with existing handlers.
   */
  virtual void HandleResponse(const std::string &result_payload, const std::string &taskId,
                              const std::string &result_id) {
    HandleResponse(result_payload, taskId);
  }

  /**
   * @brief Callback function called when a task succeeds (legacy overload).
   * @param result_payload Task result
   * @param taskId Task Id
   * @note  Override this if you do not need the result blob ID. Override the three-parameter
   *        version instead if you need to chain tasks via BlobDefinition::FromBlobId.
   */
  virtual void HandleResponse(const std::string &result_payload, const std::string &taskId) {}

  /**
   * @brief Callback function called when a tasks fails
   * @param e Risen error
   * @param taskId Task Id
   * @note  It can be called for multiple tasks in parallel, so it must be thread-safe
   */
  virtual void HandleError(const std::exception &e, const std::string &taskId) = 0;
};
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
