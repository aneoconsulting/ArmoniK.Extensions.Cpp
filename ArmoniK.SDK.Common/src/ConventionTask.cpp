#include "armonik/sdk/common/ConventionTask.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include "armonik/sdk/common/TaskDefinition.h"
#include "armonik/sdk/common/TaskOptions.h"
#include "armonik/sdk/common/internal/ConventionPayload.h"
#include <algorithm>

namespace ArmoniK {
namespace Sdk {
namespace Common {

bool ConventionTask::IsConventionTask(const TaskOptions &options) {
  return options.options.count(DynamicLibrary::KeyConventionVersion) != 0;
}

namespace {
/**
 * @brief Outputs of a convention task: those of the payload, which must be exactly the expected results
 */
std::map<std::string, std::string> ResolveOutputs(std::map<std::string, std::string> payload_outputs,
                                                  const std::vector<std::string> &expected) {
  if (payload_outputs.empty()) {
    if (expected.size() != 1) {
      throw ArmoniKSdkException("Convention payload declares no output, and the task expects " +
                                std::to_string(expected.size()) + " results instead of 1");
    }
    return {{TaskDefinition::DefaultOutputName, expected[0]}};
  }
  std::vector<std::string> expected_ids(expected);
  std::sort(expected_ids.begin(), expected_ids.end());
  std::vector<std::string> output_ids;
  for (const auto &kv : payload_outputs) {
    if (!std::binary_search(expected_ids.begin(), expected_ids.end(), kv.second)) {
      throw ArmoniKSdkException("Output '" + kv.first + "' (" + kv.second + ") is not an expected result of the task");
    }
    output_ids.push_back(kv.second);
  }
  std::sort(output_ids.begin(), output_ids.end());
  const auto duplicate = std::adjacent_find(output_ids.begin(), output_ids.end());
  if (duplicate != output_ids.end()) {
    throw ArmoniKSdkException("Result " + *duplicate + " is given to several outputs");
  }
  for (const auto &id : expected_ids) {
    if (!std::binary_search(output_ids.begin(), output_ids.end(), id)) {
      throw ArmoniKSdkException("Expected result " + id + " is not an output in the convention payload");
    }
  }
  return payload_outputs;
}
} // namespace

ConventionTask ConventionTask::Decode(const TaskOptions &options, const std::string &payload,
                                      const std::map<std::string, std::string> &data_dependencies,
                                      const std::vector<std::string> &expected_results) {
  const auto version = options.GetConventionVersion();
  if (version != DynamicLibrary::ConventionVersion) {
    throw ArmoniKSdkException("Unsupported convention version: " + version);
  }

  ConventionTask task;
  task.method_name = options.GetSymbol();

  auto decoded = ConventionPayload::Deserialize(payload);
  task.outputs = ResolveOutputs(std::move(decoded.outputs), expected_results);

  // Resolve inputs: if a value matches a data dependency key (blob ID), substitute its downloaded content.
  // This handles both inline values (C++ native payloads) and blob ID references (cross-SDK interoperability).
  for (auto &pair : decoded.inputs) {
    const auto dep_it = data_dependencies.find(pair.second);
    if (dep_it != data_dependencies.end()) {
      task.inputs[pair.first] = dep_it->second;
      task.input_ids[pair.first] = std::move(pair.second);
    } else {
      task.inputs[pair.first] = std::move(pair.second);
    }
  }
  return task;
}

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
