#pragma once

#include <armonik/sdk/common/BlobDefinition.h>
#include <armonik/sdk/common/TaskDefinition.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Worker {

/**
 * @brief Operations of the running task on the ArmoniK agent.
 *
 * The worker that hosts the service implements it (ConventionExecutor does it with the TaskHandler of
 * ArmoniK.Api.Worker), and TaskContext builds the convention on top of it. Services use TaskContext, not this class.
 */
class TaskAgent {
public:
  /**
   * @brief Data to write to a result. The data must outlive the call.
   */
  struct ResultData {
    std::string id;
    const char *data;
    std::size_t size;
  };

  /**
   * @brief Task to submit
   */
  struct TaskRequest {
    std::string payload;
    std::vector<std::string> data_dependencies;
    std::vector<std::string> expected_outputs;
  };

  virtual ~TaskAgent() = default;

  /**
   * @brief Id of the session of the running task
   */
  virtual const std::string &session_id() const = 0;

  /**
   * @brief Id of the running task
   */
  virtual const std::string &task_id() const = 0;

  /**
   * @brief Options of the running task
   */
  virtual const Common::TaskOptions &task_options() const = 0;

  /**
   * @brief Creates results without data
   * @param names Result names
   * @return Result ids, in the order of names
   */
  virtual std::vector<std::string> create_results_metadata(const std::vector<std::string> &names) = 0;

  /**
   * @brief Writes the data of results that the running task created or must produce
   * @param results Result ids and data
   */
  virtual void send_results(const std::vector<ResultData> &results) = 0;

  /**
   * @brief Submits tasks that share the same options
   * @param options Options of the tasks
   * @param tasks Tasks to submit
   * @return Task ids, in the order of tasks
   */
  virtual std::vector<std::string> submit_tasks(const Common::TaskOptions &options,
                                                const std::vector<TaskRequest> &tasks) = 0;
};

class TaskContext;

/**
 * @brief An input of the running task
 */
class TaskInput {
public:
  /**
   * @brief Blob id of the input, empty when the value was given inline in the payload
   */
  const std::string &id() const { return id_; }

  /**
   * @brief Content of the input
   */
  const std::string &data() const { return *data_; }

  /**
   * @brief The input as a subtask input: a reference to its blob, or its data when it has no blob
   */
  Common::BlobDefinition AsBlobDefinition() const {
    return id_.empty() ? Common::BlobDefinition::FromData(*data_) : Common::BlobDefinition::FromBlobId(id_);
  }

private:
  friend class TaskContext;
  TaskInput(std::string id, const std::string *data) : id_(std::move(id)), data_(data) {}

  std::string id_;
  const std::string *data_;
};

/**
 * @brief An output of the running task. The task writes it, or delegates it to a subtask with
 * Common::TaskDefinition::WithOutput(name, output.id()), but not both.
 */
class TaskOutput {
public:
  /**
   * @brief Name of the output
   */
  const std::string &name() const { return name_; }

  /**
   * @brief Blob id of the output
   */
  const std::string &id() const { return id_; }

  /**
   * @brief Writes the output
   * @param data Content of the output
   * @throws ArmoniKSdkException if the output was already written or delegated
   */
  void write(const std::string &data) const;

  // An output stays bound to its name and blob id
  TaskOutput &operator=(const TaskOutput &) = delete;

private:
  friend class TaskContext;
  TaskOutput(TaskContext &context, std::string name, std::string id)
      : context_(&context), name_(std::move(name)), id_(std::move(id)) {}

  TaskContext *context_;
  std::string name_;
  std::string id_;
};

/**
 * @brief Execution context of a convention task, given to ServiceBase::process.
 *
 * It gives the task's inputs and outputs, and lets the service create blobs and submit subtasks.
 * Each output must be written or delegated to a subtask before process returns.
 *
 * Results and subtasks created by the task are committed by ArmoniK only if the task succeeds.
 * The context is not thread-safe, and it is valid only during the call to process.
 */
class TaskContext {
public:
  /**
   * @brief Overload of ServiceBase::call used by the default ServiceBase::process. Each host keeps the overload it
   * called before ServiceBase::process existed.
   */
  enum class DefaultCall {
    /** call(session_ctx, name, inputs), as ConventionExecutor did */
    Inputs,
    /** call(session_ctx, name, payload) with the convention payload, as the DynamicWorker did through armonik_call */
    Payload,
  };

  /**
   * @brief Creates the context of a task. Called by the worker that hosts the service.
   * @param agent Operations of the task on the agent. Must outlive the context.
   * @param input_values Content of the inputs (name → data)
   * @param input_ids Blob id of the inputs given by reference (name → blob id)
   * @param outputs Outputs of the task (name → blob id)
   * @param default_call Overload of ServiceBase::call used by the default ServiceBase::process
   */
  TaskContext(TaskAgent &agent, std::map<std::string, std::string> input_values,
              const std::map<std::string, std::string> &input_ids, const std::map<std::string, std::string> &outputs,
              DefaultCall default_call = DefaultCall::Inputs);

  TaskContext(const TaskContext &) = delete;
  TaskContext &operator=(const TaskContext &) = delete;

  /**
   * @brief Id of the session of the task
   */
  const std::string &session_id() const { return agent_.session_id(); }

  /**
   * @brief Id of the task
   */
  const std::string &task_id() const { return agent_.task_id(); }

  /**
   * @brief Options of the task
   */
  const Common::TaskOptions &task_options() const { return agent_.task_options(); }

  /**
   * @brief Overload of ServiceBase::call used by the default ServiceBase::process
   */
  DefaultCall default_call() const { return default_call_; }

  /**
   * @brief Inputs of the task (name → input)
   */
  const std::map<std::string, TaskInput> &inputs() const { return inputs_; }

  /**
   * @brief Content of the inputs (name → data)
   */
  const std::map<std::string, std::string> &input_values() const { return input_values_; }

  /**
   * @brief Input of the task
   * @param name Input name
   * @throws ArmoniKSdkException if the task has no such input
   */
  const TaskInput &input(const std::string &name) const;

  /**
   * @brief Outputs of the task (name → output)
   */
  const std::map<std::string, TaskOutput> &outputs() const { return outputs_; }

  /**
   * @brief Output of the task
   * @param name Output name
   * @throws ArmoniKSdkException if the task has no such output
   */
  const TaskOutput &output(const std::string &name) const;

  /**
   * @brief Creates blobs with their data
   * @param blobs Blob names and data
   * @return Blob ids, in the order of blobs, to pass to Common::BlobDefinition::FromBlobId
   */
  std::vector<std::string> create_blobs(const std::vector<std::pair<std::string, std::string>> &blobs);

  /**
   * @brief Creates blobs without data, for example outputs that several subtasks share as an input.
   * Each blob must be given to a subtask as an existing output before process returns: nothing else can write it.
   * @param names Blob names
   * @return Blob ids, in the order of names
   */
  std::vector<std::string> create_blobs_metadata(const std::vector<std::string> &names);

  /**
   * @brief Options for subtasks that call another method: the options of the task, with the Symbol replaced
   * @param method Name of the method the subtasks call
   */
  Common::TaskOptions subtask_options(const std::string &method) const;

  /**
   * @brief Submits subtasks with the options of the task, so they call the same method of the same library
   * @param definitions Subtask definitions
   * @return Task ids, in the order of definitions
   */
  std::vector<std::string> submit_tasks(const std::vector<Common::TaskDefinition> &definitions);

  /**
   * @brief Submits subtasks
   * @param definitions Subtask definitions. An existing output that is an output of the task delegates it.
   * @param options Options of the subtasks. They are marked as convention tasks if they are not.
   * @return Task ids, in the order of definitions
   * @throws ArmoniKSdkException if an output of the task is delegated twice, or written and delegated, or if a blob
   * created by create_blobs_metadata is given to two subtasks
   */
  std::vector<std::string> submit_tasks(const std::vector<Common::TaskDefinition> &definitions,
                                        const Common::TaskOptions &options);

  /**
   * @brief Checks that every output was written or delegated, and that every blob created by create_blobs_metadata
   * was given to a subtask. Called by the worker after ServiceBase::process.
   * @throws ArmoniKSdkException if an output was neither written nor delegated, or a blob was given to no subtask
   */
  void finish() const;

private:
  friend class TaskOutput;

  enum class OutputState { Pending, Written, Delegated };

  void write_output(const std::string &name, const std::string &data);
  std::vector<std::string> create_results(const std::vector<std::string> &names);

  TaskAgent &agent_;
  DefaultCall default_call_;
  std::map<std::string, std::string> input_values_;
  std::map<std::string, TaskInput> inputs_;
  std::map<std::string, TaskOutput> outputs_;
  std::map<std::string, OutputState> output_states_;
  /** Blobs created by create_blobs_metadata that no subtask produces yet (blob id → name) */
  std::map<std::string, std::string> unassigned_blobs_;
  /** Blobs created by create_blobs_metadata that a subtask produces */
  std::set<std::string> assigned_blobs_;
};

} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
