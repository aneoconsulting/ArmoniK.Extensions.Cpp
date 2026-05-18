#pragma once

#include <stdexcept>
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
   * Override this version. The default delegates to the deprecated two-parameter overload for
   * backward compatibility with existing handlers that already override it.
   */
  virtual void HandleResponse(const std::string &result_payload, const std::string &taskId,
                              const std::string &result_id) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    HandleResponse(result_payload, taskId);
#pragma GCC diagnostic pop
  }

  /**
   * @brief Legacy callback overload — override the three-parameter version instead.
   * @deprecated Override HandleResponse(result_payload, taskId, result_id) instead.
   *             This overload exists only for backward compatibility and will be removed in a future release.
   */
  [[deprecated("Override HandleResponse(result_payload, taskId, result_id) instead")]]
  virtual void HandleResponse(const std::string &result_payload, const std::string &taskId) {
    (void)result_payload;
    (void)taskId;
    throw std::logic_error(
        "HandleResponse not implemented — override HandleResponse(result_payload, taskId, result_id)");
  }

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
