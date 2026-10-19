#include "armonik/sdk/common/ConventionTask.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include "armonik/sdk/common/TaskOptions.h"
#include "armonik/sdk/common/internal/ConventionPayload.h"

namespace ArmoniK {
namespace Sdk {
namespace Common {

bool ConventionTask::IsConventionTask(const TaskOptions &options) {
  return options.options.count(DynamicLibrary::KeyConventionVersion) != 0;
}

ConventionTask ConventionTask::Decode(const TaskOptions &options, const std::string &payload,
                                      const std::map<std::string, std::string> &data_dependencies) {
  const auto version = options.GetConventionVersion();
  if (version != DynamicLibrary::ConventionVersion) {
    throw ArmoniKSdkException("Unsupported convention version: " + version);
  }

  ConventionTask task;
  task.method_name = options.GetSymbol();

  auto decoded = ConventionPayload::Deserialize(payload);
  task.outputs = std::move(decoded.outputs);

  // Resolve inputs: if a value matches a data dependency key (blob ID), substitute its downloaded content.
  // This handles both inline values (C++ native payloads) and blob ID references (cross-SDK interoperability).
  for (auto &pair : decoded.inputs) {
    const auto dep_it = data_dependencies.find(pair.second);
    task.inputs[pair.first] = (dep_it != data_dependencies.end()) ? dep_it->second : std::move(pair.second);
  }
  return task;
}

} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
