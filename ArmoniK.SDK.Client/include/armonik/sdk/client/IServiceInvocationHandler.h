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
};
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
