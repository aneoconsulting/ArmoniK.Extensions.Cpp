#pragma once

#include "ArmoniKSdkException.h"
#include "BlobDefinition.h"
#include <map>
#include <set>
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
 * Outputs are declared by name. The SDK creates one result per new output and writes the
 * name → result ID mapping in the payload, as the other ArmoniK SDKs do. An output can also be
 * a blob that already exists in the session, given by ID: on a worker, this is how a task
 * delegates one of its own outputs to a subtask. When no output is declared, a single output
 * named DefaultOutputName is created.
 *
 * A blob is the output of a single task. Submit rejects an existing output that another task
 * submitted through the same SessionService is still waiting for. This check is local to that
 * SessionService: it does not see other clients of the session, nor outputs already handled.
 *
 * The method to call on the worker is not part of the task definition: it is selected by the
 * DynamicLibrary::symbol task option.
 *
 * Example:
 * @code
 * service.Submit({
 *   TaskDefinition({
 *     {"param_a", BlobDefinition::FromData(raw_bytes_a)},
 *     {"param_b", BlobDefinition::FromBlobId(existing_blob_id)},
 *   }, {"quotient", "remainder"})
 * }, handler);
 * @endcode
 */
struct TaskDefinition {
  /**
   * @brief Name of the output created when a task definition declares no output
   */
  static constexpr const char *DefaultOutputName = "result";

  TaskDefinition() = default;

  /**
   * @brief Constructs a task definition
   * @param inputs_ Named inputs
   * @param outputs_ Output names. If empty, a single output named DefaultOutputName is created on submission
   */
  explicit TaskDefinition(std::map<std::string, BlobDefinition> inputs_, const std::set<std::string> &outputs_ = {})
      : inputs(std::move(inputs_)) {
    for (const auto &name : outputs_) {
      WithOutput(name);
    }
  }

  /**
   * @brief Named inputs: maps a user-defined name to a BlobDefinition (raw data or blob ref)
   */
  std::map<std::string, BlobDefinition> inputs;

  /**
   * @brief Outputs the worker must produce (name → blob ID). An empty blob ID declares a new output, created on
   * submission. A non-empty one is an output that already exists in the session.
   */
  std::map<std::string, std::string> outputs;

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

  /**
   * @brief Adds a new named output to this task definition. Declaring the same new output twice has no effect.
   * @param name Output name
   * @return *this for chaining
   * @throws ArmoniKSdkException if the name is empty or already declared as an existing output
   */
  TaskDefinition &WithOutput(std::string name) { return AddOutput(std::move(name), std::string()); }

  /**
   * @brief Adds a named output that already exists in the session. Declaring the same output twice with the same
   * blob ID has no effect.
   * @param name Output name
   * @param blob_id Blob ID of the existing output, for example TaskOutput::id() to delegate an output of the
   *        current task to this subtask
   * @return *this for chaining
   * @throws ArmoniKSdkException if the name or the blob ID is empty, or if the name is already declared otherwise
   */
  TaskDefinition &WithOutput(std::string name, std::string blob_id) {
    if (blob_id.empty()) {
      throw ArmoniKSdkException("Existing output '" + name + "' has an empty blob ID");
    }
    return AddOutput(std::move(name), std::move(blob_id));
  }

private:
  TaskDefinition &AddOutput(std::string name, std::string blob_id) {
    if (name.empty()) {
      throw ArmoniKSdkException("Output name is empty");
    }
    const auto it = outputs.find(name);
    if (it == outputs.end()) {
      outputs.emplace(std::move(name), std::move(blob_id));
    } else if (it->second != blob_id) {
      throw ArmoniKSdkException("Output '" + name + "' is already declared " +
                                (it->second.empty() ? std::string("as a new output") : "with blob " + it->second));
    }
    return *this;
  }
};

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
