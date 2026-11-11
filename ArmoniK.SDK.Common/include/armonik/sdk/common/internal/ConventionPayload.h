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
 * @throws ArmoniKSdkException if an output name is empty, if an existing blob is given as an output twice, or if a
 *         task depends, directly or through other tasks of the batch, on one of its own existing outputs
 */
inline void ValidateConventionOutputs(const std::vector<TaskDefinition> &definitions) {
  // Task producing each existing output
  std::map<std::string, std::size_t> producers;
  for (std::size_t i = 0; i < definitions.size(); ++i) {
    for (const auto &kv : definitions[i].outputs) {
      if (kv.first.empty()) {
        throw ArmoniKSdkException("Output name is empty");
      }
      if (!kv.second.empty() && !producers.emplace(kv.second, i).second) {
        throw ArmoniKSdkException("Blob " + kv.second + " is given as an output twice: a result has a single owner");
      }
    }
  }
  if (producers.empty()) {
    return;
  }

  // A task that reads an existing output of the batch runs after the task that produces it. A cycle in this order
  // means that the tasks wait for each other forever.
  std::vector<std::vector<std::size_t>> successors(definitions.size());
  std::vector<std::size_t> predecessor_count(definitions.size(), 0);
  for (std::size_t i = 0; i < definitions.size(); ++i) {
    for (const auto &kv : definitions[i].inputs) {
      if (kv.second.IsRawData()) {
        continue;
      }
      const auto producer = producers.find(kv.second.GetBlobId());
      if (producer == producers.end()) {
        continue;
      }
      if (producer->second == i) {
        throw ArmoniKSdkException("Input '" + kv.first + "' is the blob " + producer->first +
                                  ", which is an output of the same task: the task would never run");
      }
      successors[producer->second].push_back(i);
      ++predecessor_count[i];
    }
  }
  std::vector<std::size_t> ready;
  for (std::size_t i = 0; i < definitions.size(); ++i) {
    if (predecessor_count[i] == 0) {
      ready.push_back(i);
    }
  }
  std::size_t ordered = 0;
  while (!ready.empty()) {
    const auto i = ready.back();
    ready.pop_back();
    ++ordered;
    for (const auto j : successors[i]) {
      if (--predecessor_count[j] == 0) {
        ready.push_back(j);
      }
    }
  }
  if (ordered != definitions.size()) {
    std::string tasks;
    for (std::size_t i = 0; i < definitions.size(); ++i) {
      if (predecessor_count[i] != 0) {
        tasks += (tasks.empty() ? "" : ", ") + std::to_string(i);
      }
    }
    throw ArmoniKSdkException("Task definitions " + tasks +
                              " read each other's outputs in a cycle: these tasks would never run");
  }
}

/**
 * @brief Names of the outputs to create on submission: the new outputs of the definition, or a single output named
 * TaskDefinition::DefaultOutputName when it declares no output at all
 * @param definition Task definition
 */
inline std::vector<std::string> ConventionOutputsToCreate(const TaskDefinition &definition) {
  if (definition.outputs.empty()) {
    // Built from the pointer value: in C++11, binding a reference to the constexpr member needs its definition,
    // which is in ArmoniK.SDK.Common, not linked into worker libraries
    return {std::string(TaskDefinition::DefaultOutputName)};
  }
  std::vector<std::string> names;
  for (const auto &kv : definition.outputs) {
    if (kv.second.empty()) {
      names.push_back(kv.first);
    }
  }
  return names;
}

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
