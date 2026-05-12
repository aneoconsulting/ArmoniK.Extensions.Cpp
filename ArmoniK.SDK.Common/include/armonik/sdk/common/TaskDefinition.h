#pragma once

#include "BlobDefinition.h"
#include <map>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief User-facing task descriptor for the convention execution path.
 *
 * Unlike TaskPayload (which requires callers to pre-allocate result IDs for every input),
 * TaskDefinition accepts raw data directly. The SDK uploads all raw inputs on the caller's
 * behalf during Submit(), so callers only need to provide the data they want to send.
 *
 * Inputs that were already uploaded (e.g. a shared large dataset) can be referenced by blob
 * ID via BlobDefinition::FromBlobId() to avoid re-uploading.
 *
 * Example:
 * @code
 * service.Submit({
 *   TaskDefinition{"my_method", {
 *     {"param_a", BlobDefinition::FromData(raw_bytes_a)},
 *     {"param_b", BlobDefinition::FromBlobId(existing_blob_id)},
 *   }}
 * }, handler);
 * @endcode
 */
struct TaskDefinition {
  TaskDefinition() = default;

  TaskDefinition(std::string method_name_, std::map<std::string, BlobDefinition> inputs_)
      : method_name(std::move(method_name_)), inputs(std::move(inputs_)) {}

  /**
   * @brief Method name to dispatch to on the worker
   */
  std::string method_name;

  /**
   * @brief Named inputs: maps a user-defined name to a BlobDefinition (raw data or blob ref)
   */
  std::map<std::string, BlobDefinition> inputs;

  /**
   * @brief Adds a named input to this task definition.
   * @param name Input name
   * @param blob Blob definition (raw data or existing blob ID)
   * @return *this for chaining
   */
  TaskDefinition &WithInput(std::string name, BlobDefinition blob) {
    inputs.emplace(std::move(name), std::move(blob));
    return *this;
  }
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
