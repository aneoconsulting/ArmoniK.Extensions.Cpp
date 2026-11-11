#include "armonik/sdk/worker/TaskContext.h"
#include "armonik/sdk/common/ArmoniKSdkException.h"
#include "armonik/sdk/common/DynamicLibrary.h"
#include "armonik/sdk/common/internal/ConventionPayload.h"
#include <algorithm>
#include <set>

namespace ArmoniK {
namespace Sdk {
namespace Worker {

namespace {
/**
 * @brief Removes the repeated ids, keeping the first occurrence of each
 */
std::vector<std::string> WithoutDuplicates(std::vector<std::string> ids) {
  std::set<std::string> seen;
  std::vector<std::string> unique;
  unique.reserve(ids.size());
  for (auto &id : ids) {
    if (seen.insert(id).second) {
      unique.push_back(std::move(id));
    }
  }
  return unique;
}
} // namespace

void TaskOutput::write(const std::string &data) const { context_->write_output(name_, data); }

TaskContext::TaskContext(TaskAgent &agent, std::map<std::string, std::string> input_values,
                         const std::map<std::string, std::string> &input_ids,
                         const std::map<std::string, std::string> &outputs, DefaultCall default_call)
    : agent_(agent), default_call_(default_call), input_values_(std::move(input_values)) {
  // Map nodes are stable, so the inputs can point to the values
  for (const auto &kv : input_values_) {
    const auto id = input_ids.find(kv.first);
    inputs_.emplace(kv.first, TaskInput(id == input_ids.end() ? std::string() : id->second, &kv.second));
  }
  for (const auto &kv : outputs) {
    outputs_.emplace(kv.first, TaskOutput(*this, kv.first, kv.second));
    output_states_.emplace(kv.first, OutputState::Pending);
  }
}

const TaskInput &TaskContext::input(const std::string &name) const {
  const auto it = inputs_.find(name);
  if (it == inputs_.end()) {
    throw Common::ArmoniKSdkException("Task has no input '" + name + "'");
  }
  return it->second;
}

const TaskOutput &TaskContext::output(const std::string &name) const {
  const auto it = outputs_.find(name);
  if (it == outputs_.end()) {
    throw Common::ArmoniKSdkException("Task has no output '" + name + "'");
  }
  return it->second;
}

void TaskContext::write_output(const std::string &name, const std::string &data) {
  auto &state = output_states_.at(name);
  if (state == OutputState::Written) {
    throw Common::ArmoniKSdkException("Output '" + name + "' is already written");
  }
  if (state == OutputState::Delegated) {
    throw Common::ArmoniKSdkException("Output '" + name + "' is delegated to a subtask");
  }
  agent_.send_results({{outputs_.at(name).id(), data.data(), data.size()}});
  state = OutputState::Written;
}

std::vector<std::string> TaskContext::create_blobs(const std::vector<std::pair<std::string, std::string>> &blobs) {
  std::vector<std::string> names;
  names.reserve(blobs.size());
  for (const auto &blob : blobs) {
    names.push_back(blob.first);
  }
  auto ids = create_results(names);

  std::vector<TaskAgent::ResultData> data;
  data.reserve(blobs.size());
  for (std::size_t i = 0; i < blobs.size(); ++i) {
    data.push_back({ids[i], blobs[i].second.data(), blobs[i].second.size()});
  }
  agent_.send_results(data);
  return ids;
}

std::vector<std::string> TaskContext::create_blobs_metadata(const std::vector<std::string> &names) {
  auto ids = create_results(names);
  for (std::size_t i = 0; i < ids.size(); ++i) {
    unassigned_blobs_.emplace(ids[i], names[i]);
  }
  return ids;
}

std::vector<std::string> TaskContext::create_results(const std::vector<std::string> &names) {
  if (names.empty()) {
    return {};
  }
  auto ids = agent_.create_results_metadata(names);
  if (ids.size() != names.size()) {
    throw Common::ArmoniKSdkException("Created " + std::to_string(ids.size()) + " results instead of " +
                                      std::to_string(names.size()));
  }
  return ids;
}

Common::TaskOptions TaskContext::subtask_options(const std::string &method) const {
  auto options = task_options();
  options.options[Common::DynamicLibrary::KeySymbol] = method;
  return options;
}

std::vector<std::string> TaskContext::submit_tasks(const std::vector<Common::TaskDefinition> &definitions) {
  return submit_tasks(definitions, task_options());
}

std::vector<std::string> TaskContext::submit_tasks(const std::vector<Common::TaskDefinition> &definitions,
                                                   const Common::TaskOptions &options) {
  if (definitions.empty()) {
    return {};
  }

  auto subtask_options = options;
  if (subtask_options.options.count(Common::DynamicLibrary::KeyConventionVersion) == 0) {
    subtask_options.options[Common::DynamicLibrary::KeyConventionVersion] = Common::DynamicLibrary::ConventionVersion;
  }
  const auto symbol = subtask_options.options.find(Common::DynamicLibrary::KeySymbol);
  if (symbol == subtask_options.options.end() || symbol->second.empty()) {
    throw Common::ArmoniKSdkException(std::string("Subtask options have no '") + Common::DynamicLibrary::KeySymbol +
                                      "' option: the subtasks would not know which method to call");
  }

  Common::ValidateConventionOutputs(definitions);

  // Find the outputs of this task that the subtasks take over, before anything is created
  std::map<std::string, std::string> own_outputs; // blob id → output name
  for (const auto &kv : outputs_) {
    own_outputs.emplace(kv.second.id(), kv.first);
  }
  // A subtask can produce an output of this task, or a blob created by create_blobs_metadata: nothing else is
  // known to be free, and Core transfers to a subtask only the outputs of its parent
  std::vector<std::string> delegated;
  std::vector<std::string> assigned;
  // The subtasks that produce such blobs, with only what orders them: their blob inputs and existing outputs
  std::vector<Common::TaskDefinition> producers;
  for (const auto &definition : definitions) {
    Common::TaskDefinition producer;
    for (const auto &kv : definition.outputs) {
      if (kv.second.empty()) {
        continue; // A new output, created below
      }
      producer.outputs.insert(kv);
      const auto own = own_outputs.find(kv.second);
      if (own != own_outputs.end()) {
        const auto state = output_states_.at(own->second);
        if (state == OutputState::Written) {
          throw Common::ArmoniKSdkException("Output '" + own->second + "' is already written and cannot be delegated");
        }
        if (state == OutputState::Delegated) {
          throw Common::ArmoniKSdkException("Output '" + own->second + "' is already delegated to a subtask");
        }
        delegated.push_back(own->second);
      } else if (assigned_blobs_.count(kv.second) != 0) {
        throw Common::ArmoniKSdkException("Blob " + kv.second + " is already the output of a subtask");
      } else if (unassigned_blobs_.count(kv.second) != 0) {
        assigned.push_back(kv.second);
      } else {
        throw Common::ArmoniKSdkException("Blob " + kv.second +
                                          " is neither an output of the task nor a blob created by "
                                          "create_blobs_metadata: a subtask cannot produce it");
      }
    }
    if (producer.outputs.empty()) {
      continue;
    }
    for (const auto &kv : definition.inputs) {
      if (!kv.second.IsRawData()) {
        producer.inputs.insert(kv);
      }
    }
    producers.push_back(std::move(producer));
  }
  // A cycle goes only through subtasks that produce such blobs. ValidateConventionOutputs found none within this call,
  // so look for one through the producers of earlier calls.
  if (!producers.empty() && !producers_.empty()) {
    auto all = producers_;
    all.insert(all.end(), producers.begin(), producers.end());
    try {
      Common::ValidateConventionOutputs(all);
    } catch (const Common::ArmoniKSdkException &) {
      throw Common::ArmoniKSdkException("Subtasks read each other's outputs in a cycle with subtasks of an earlier "
                                        "submit_tasks call: these subtasks would never run");
    }
  }

  // Create the raw inputs and the new outputs of every subtask in a single call
  struct Created {
    std::size_t task_idx;
    std::string name;
    bool is_input;
  };
  std::vector<Created> created;
  std::vector<std::string> names;
  for (std::size_t i = 0; i < definitions.size(); ++i) {
    const auto &definition = definitions[i];
    for (const auto &kv : definition.inputs) {
      if (kv.second.IsRawData()) {
        created.push_back({i, kv.first, true});
        names.push_back(kv.first);
      }
    }
    for (auto &name : Common::ConventionOutputsToCreate(definition)) {
      names.push_back(name);
      created.push_back({i, std::move(name), false});
    }
  }
  const auto ids = create_results(names);

  std::vector<Common::ConventionPayload> payloads(definitions.size());
  std::vector<TaskAgent::TaskRequest> requests(definitions.size());
  std::vector<TaskAgent::ResultData> input_data;
  for (std::size_t j = 0; j < created.size(); ++j) {
    const auto &c = created[j];
    if (c.is_input) {
      payloads[c.task_idx].inputs[c.name] = ids[j];
      requests[c.task_idx].data_dependencies.push_back(ids[j]);
      const auto &data = definitions[c.task_idx].inputs.at(c.name).GetData();
      input_data.push_back({ids[j], data.data(), data.size()});
    } else {
      payloads[c.task_idx].outputs[c.name] = ids[j];
      requests[c.task_idx].expected_outputs.push_back(ids[j]);
    }
  }
  if (!input_data.empty()) {
    agent_.send_results(input_data);
  }

  // A subtask of a library uploaded as a blob depends on that blob, as the client does
  const auto library_blob = subtask_options.options.find(Common::DynamicLibrary::KeyLibraryBlobId);
  for (std::size_t i = 0; i < definitions.size(); ++i) {
    const auto &definition = definitions[i];
    for (const auto &kv : definition.inputs) {
      if (!kv.second.IsRawData()) {
        payloads[i].inputs[kv.first] = kv.second.GetBlobId();
        requests[i].data_dependencies.push_back(kv.second.GetBlobId());
      }
    }
    for (const auto &kv : definition.outputs) {
      if (kv.second.empty()) {
        continue; // A new output, created below
      }
      payloads[i].outputs[kv.first] = kv.second;
      requests[i].expected_outputs.push_back(kv.second);
    }
    if (library_blob != subtask_options.options.end() && !library_blob->second.empty()) {
      requests[i].data_dependencies.push_back(library_blob->second);
    }
    // Core rejects a task that lists a pending dependency twice: two inputs may share a blob
    requests[i].data_dependencies = WithoutDuplicates(std::move(requests[i].data_dependencies));
    requests[i].payload = payloads[i].Serialize();
  }

  // Mark the outputs and blobs before submitting: after a transport failure, the agent may have registered the
  // subtasks, which would then produce them, so the task must not write them anymore
  for (const auto &name : delegated) {
    output_states_.at(name) = OutputState::Delegated;
  }
  for (const auto &id : assigned) {
    unassigned_blobs_.erase(id);
    assigned_blobs_.insert(id);
  }
  producers_.insert(producers_.end(), producers.begin(), producers.end());
  try {
    return agent_.submit_tasks(subtask_options, requests);
  } catch (...) {
    // The outputs are marked as delegated whether the subtasks exist or not: finish fails the task even if the
    // service catches this exception
    if (!submit_failure_) {
      submit_failure_ = std::current_exception();
    }
    throw;
  }
}

void TaskContext::finish() const {
  if (submit_failure_) {
    std::rethrow_exception(submit_failure_);
  }
  std::string pending;
  for (const auto &kv : output_states_) {
    if (kv.second == OutputState::Pending) {
      pending += (pending.empty() ? "'" : ", '") + kv.first + "'";
    }
  }
  if (!pending.empty()) {
    throw Common::ArmoniKSdkException("Outputs neither written nor delegated to a subtask: " + pending);
  }
  std::string unassigned;
  for (const auto &kv : unassigned_blobs_) {
    unassigned += (unassigned.empty() ? "'" : ", '") + kv.second + "' (" + kv.first + ")";
  }
  if (!unassigned.empty()) {
    throw Common::ArmoniKSdkException("Blobs created without data and given to no subtask as an output: " + unassigned);
  }
}

} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
