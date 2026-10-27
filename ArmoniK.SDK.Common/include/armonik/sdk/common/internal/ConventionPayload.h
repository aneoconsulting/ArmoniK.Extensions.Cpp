#pragma once

#include <armonik/common/utils/string_view.h>
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/TaskDefinition.h>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Common {

/**
 * @brief Convention task payload using JSON encoding.
 *
 * Internal wire format for the convention execution path, shared with the other ArmoniK SDKs.
 * Serialized format: {"inputs":{"<name>":"<blob id>",...},"outputs":{"<name>":"<blob id>",...}}
 *
 * Header-only, so that worker libraries, which do not link ArmoniK.SDK.Common, submit subtasks with the same
 * encoding as the client. Including it requires nlohmann_json.
 *
 * @note This is an internal SDK type. It is not part of the public API and
 *       may change or be removed in any future release without notice.
 */
struct ConventionPayload {
  ConventionPayload() = default;

  std::map<std::string, std::string> inputs;
  std::map<std::string, std::string> outputs;

  /**
   * @throws ArmoniKSdkException if a value is not valid UTF-8
   */
  [[nodiscard]] std::string Serialize() const {
    try {
      nlohmann::json j;
      j["inputs"] = inputs;
      j["outputs"] = outputs;
      return j.dump();
    } catch (const nlohmann::json::exception &e) {
      throw ArmoniKSdkException(std::string("Failed to serialize convention payload JSON: ") + e.what());
    }
  }

  static ConventionPayload Deserialize(armonik::api::string_view serialized) {
    try {
      auto j = nlohmann::json::parse(serialized.begin(), serialized.end());
      ConventionPayload payload;
      payload.inputs = j.at("inputs").get<std::map<std::string, std::string>>();
      // Tolerate a missing "outputs" field: the worker then falls back to the task's expected outputs
      const auto outputs = j.find("outputs");
      if (outputs != j.end()) {
        payload.outputs = outputs->get<std::map<std::string, std::string>>();
      }
      return payload;
    } catch (const nlohmann::json::exception &e) {
      throw ArmoniKSdkException(std::string("Failed to deserialize convention payload JSON: ") + e.what());
    }
  }
};

/**
 * @brief Checks the outputs of task definitions submitted together, before anything is created
 * @param definitions Task definitions
 * @throws ArmoniKSdkException if an output is declared both as a new and an existing output, or if an existing
 *         blob is given as an output twice
 */
inline void ValidateConventionOutputs(const std::vector<TaskDefinition> &definitions) {
  std::set<std::string> existing_ids;
  for (const auto &definition : definitions) {
    for (const auto &kv : definition.existing_outputs) {
      if (definition.outputs.count(kv.first) != 0) {
        throw ArmoniKSdkException("Output '" + kv.first + "' is declared both as a new and an existing output");
      }
      if (!existing_ids.insert(kv.second).second) {
        throw ArmoniKSdkException("Blob " + kv.second + " is given as an output twice: a result has a single owner");
      }
    }
  }
}

/**
 * @brief Names of the outputs to create on submission: the new outputs of the definition, or a single output named
 * TaskDefinition::DefaultOutputName when it declares no output at all
 * @param definition Task definition
 */
inline std::vector<std::string> ConventionOutputsToCreate(const TaskDefinition &definition) {
  if (definition.outputs.empty() && definition.existing_outputs.empty()) {
    // Built from the pointer value: in C++11, binding a reference to the constexpr member needs its definition,
    // which is in ArmoniK.SDK.Common, not linked into worker libraries
    return {std::string(TaskDefinition::DefaultOutputName)};
  }
  return {definition.outputs.begin(), definition.outputs.end()};
}

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
