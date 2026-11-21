#pragma once

#include "../ArmoniKSDKInterface.h"
#include "../TaskContext.h"
#include "TaskOptionsJson.h"
#include <algorithm>
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <cstring>
#include <exception>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Internal {

/**
 * @brief Worker side of armonik_process: exposes a task and its TaskAgent to a library as an armonik_task_t
 *
 * It tracks the outputs of the task and the results the library creates, so that the worker checks them whatever
 * armonik_process the library defines: each is written once or produced by a subtask, not both, and the outputs must
 * all be produced. The library writes, and gives to subtasks as outputs, only these results. Unlike TaskContext, the
 * host does not look for subtasks that read each other's outputs in a cycle.
 *
 * @note This is an internal SDK header. It is not part of the public API.
 */
class TaskApiHost {
public:
  /**
   * @brief Describes a task. Every argument must outlive this object.
   * @param agent Operations of the task
   * @param input_values Content of the inputs (name → data)
   * @param input_ids Blob id of the inputs given by reference (name → blob id)
   * @param outputs Outputs of the task (name → blob id)
   */
  TaskApiHost(TaskAgent &agent, const std::map<std::string, std::string> &input_values,
              const std::map<std::string, std::string> &input_ids, const std::map<std::string, std::string> &outputs)
      : agent_(agent), options_(TaskOptionsToJson(agent.task_options())) {
    for (const auto &kv : input_values) {
      const auto id = input_ids.find(kv.first);
      inputs_.push_back(
          {String(kv.first), id == input_ids.end() ? armonik_string_t{"", 0} : String(id->second), String(kv.second)});
    }
    for (const auto &kv : outputs) {
      outputs_.push_back({String(kv.first), String(kv.second)});
      results_.emplace(kv.second, Result{kv.first, true, ResultState::Pending});
    }
    task_.struct_size = sizeof(armonik_task_t);
    task_.agent = this;
    task_.session_id = String(agent.session_id());
    task_.task_id = String(agent.task_id());
    task_.task_options = String(options_);
    task_.inputs = inputs_.data();
    task_.inputs_count = inputs_.size();
    task_.outputs = outputs_.data();
    task_.outputs_count = outputs_.size();
    task_.create_results_metadata = &CreateResultsMetadata;
    task_.send_results = &SendResults;
    task_.submit_tasks = &SubmitTasks;
  }

  TaskApiHost(const TaskApiHost &) = delete;
  TaskApiHost &operator=(const TaskApiHost &) = delete;

  /**
   * @brief The task to give to armonik_process
   */
  const armonik_task_t &task() const { return task_; }

  /**
   * @brief Checks that every output was written or delegated, and that every result created without data was written
   * or given to a subtask. Called by the worker after armonik_process succeeds.
   * @throws ArmoniKSdkException if an output was neither written nor delegated, or a created result has no data and no
   * producer
   * @throws std::exception the exception of a failed submission, even if the library ignored the error: no subtask
   * may produce the outputs it delegated
   */
  void finish() const {
    if (submit_failure_) {
      std::rethrow_exception(submit_failure_);
    }
    std::string pending_outputs;
    std::string pending_results;
    for (const auto &kv : results_) {
      if (kv.second.state != ResultState::Pending) {
        continue;
      }
      if (kv.second.is_output) {
        pending_outputs += (pending_outputs.empty() ? "'" : ", '") + kv.second.name + "'";
      } else {
        pending_results += (pending_results.empty() ? "'" : ", '") + kv.second.name + "' (" + kv.first + ")";
      }
    }
    if (!pending_outputs.empty()) {
      throw Common::ArmoniKSdkException("Outputs neither written nor delegated to a subtask: " + pending_outputs);
    }
    if (!pending_results.empty()) {
      throw Common::ArmoniKSdkException("Results created without data and given to no subtask as an output: " +
                                        pending_results);
    }
  }

private:
  enum class ResultState { Pending, Written, Delegated };
  /** An output of the task, or a result created by the library */
  struct Result {
    std::string name;
    bool is_output;
    ResultState state;
  };

  static armonik_string_t String(const std::string &s) { return {s.data(), s.size()}; }

  /**
   * @brief Runs an operation, and reports its exception to the library: ArmoniKSdkException is a failure,
   * any other exception a transient failure. No exception crosses the C interface.
   */
  template <class F> static armonik_status_t Guard(void *sink_context, armonik_string_sink_t on_error, F operation) {
    try {
      operation();
      return ARMONIK_STATUS_OK;
    } catch (const Common::ArmoniKSdkException &e) {
      on_error(sink_context, e.what(), std::strlen(e.what()));
      return ARMONIK_STATUS_ERROR;
    } catch (const std::exception &e) {
      on_error(sink_context, e.what(), std::strlen(e.what()));
      return ARMONIK_STATUS_RETRY;
    } catch (...) {
      static const char message[] = "Unknown exception in the worker";
      on_error(sink_context, message, sizeof(message) - 1);
      return ARMONIK_STATUS_RETRY;
    }
  }

  /**
   * @brief Checks that a result can still be written or given to a subtask as an output
   * @throws ArmoniKSdkException if the id is neither an output of the task nor a result it created, or if the result is
   * already written or produced by a subtask
   */
  Result &AvailableResult(const std::string &id) {
    const auto it = results_.find(id);
    if (it == results_.end()) {
      throw Common::ArmoniKSdkException("Result " + id +
                                        " is neither an output of the task nor a result it created: the task cannot "
                                        "write it or give it to a subtask");
    }
    const auto what = (it->second.is_output ? "Output '" : "Result '") + it->second.name + "'";
    if (it->second.state == ResultState::Written) {
      throw Common::ArmoniKSdkException(what + " is already written");
    }
    if (it->second.state == ResultState::Delegated) {
      throw Common::ArmoniKSdkException(what + " is produced by a subtask");
    }
    return it->second;
  }

  static armonik_status_t CreateResultsMetadata(void *host, const armonik_string_t *names, size_t count,
                                                void *sink_context, armonik_string_sink_t on_value,
                                                armonik_string_sink_t on_error) {
    return Guard(sink_context, on_error, [&]() {
      std::vector<std::string> result_names;
      for (size_t i = 0; i < count; ++i) {
        result_names.emplace_back(names[i].data, names[i].size);
      }
      auto &self = *static_cast<TaskApiHost *>(host);
      const auto ids = self.agent_.create_results_metadata(result_names);
      if (ids.size() != result_names.size()) {
        throw Common::ArmoniKSdkException("Created " + std::to_string(ids.size()) + " results instead of " +
                                          std::to_string(result_names.size()));
      }
      for (std::size_t i = 0; i < ids.size(); ++i) {
        self.results_.emplace(ids[i], Result{result_names[i], false, ResultState::Pending});
        on_value(sink_context, ids[i].data(), ids[i].size());
      }
    });
  }

  static armonik_status_t SendResults(void *host, const armonik_result_data_t *results, size_t count,
                                      void *sink_context, armonik_string_sink_t on_error) {
    return Guard(sink_context, on_error, [&]() {
      auto &self = *static_cast<TaskApiHost *>(host);
      std::vector<TaskAgent::ResultData> data;
      std::vector<Result *> written;
      for (size_t i = 0; i < count; ++i) {
        data.push_back(
            {std::string(results[i].id.data, results[i].id.size), results[i].data.data, results[i].data.size});
        const auto result = &self.AvailableResult(data.back().id);
        if (std::find(written.begin(), written.end(), result) != written.end()) {
          throw Common::ArmoniKSdkException("Result '" + result->name + "' is written twice");
        }
        written.push_back(result);
      }
      self.agent_.send_results(data);
      for (const auto result : written) {
        result->state = ResultState::Written;
      }
    });
  }

  static armonik_status_t SubmitTasks(void *host, armonik_string_t task_options, const armonik_task_request_t *tasks,
                                      size_t count, void *sink_context, armonik_string_sink_t on_value,
                                      armonik_string_sink_t on_error) {
    return Guard(sink_context, on_error, [&]() {
      auto &self = *static_cast<TaskApiHost *>(host);
      const auto options = TaskOptionsFromJson(task_options.data, task_options.size);
      std::vector<Result *> delegated;
      std::vector<TaskAgent::TaskRequest> requests(count);
      for (size_t i = 0; i < count; ++i) {
        requests[i].payload.assign(tasks[i].payload.data, tasks[i].payload.size);
        // Core rejects a task that lists a pending dependency twice
        std::set<std::string> dependencies;
        for (size_t j = 0; j < tasks[i].data_dependencies_count; ++j) {
          std::string id(tasks[i].data_dependencies[j].data, tasks[i].data_dependencies[j].size);
          if (dependencies.insert(id).second) {
            requests[i].data_dependencies.push_back(std::move(id));
          }
        }
        for (size_t j = 0; j < tasks[i].expected_outputs_count; ++j) {
          requests[i].expected_outputs.emplace_back(tasks[i].expected_outputs[j].data,
                                                    tasks[i].expected_outputs[j].size);
          const auto result = &self.AvailableResult(requests[i].expected_outputs.back());
          if (std::find(delegated.begin(), delegated.end(), result) != delegated.end()) {
            throw Common::ArmoniKSdkException("Result '" + result->name + "' is given to two subtasks as an output");
          }
          delegated.push_back(result);
        }
      }
      // Delegated before submitting: after a transport failure, the agent may have registered the subtasks, which
      // would then produce the outputs
      for (const auto result : delegated) {
        result->state = ResultState::Delegated;
      }
      std::vector<std::string> ids;
      try {
        ids = self.agent_.submit_tasks(options, requests);
      } catch (...) {
        // finish fails the task even if the library ignores the error
        if (!self.submit_failure_) {
          self.submit_failure_ = std::current_exception();
        }
        throw;
      }
      for (const auto &id : ids) {
        on_value(sink_context, id.data(), id.size());
      }
    });
  }

  TaskAgent &agent_;
  std::string options_;
  /** Outputs of the task and results created by the library (result id → result) */
  std::map<std::string, Result> results_;
  std::vector<armonik_input_t> inputs_;
  std::vector<armonik_output_t> outputs_;
  armonik_task_t task_{};
  /** First exception of agent_.submit_tasks, rethrown by finish */
  std::exception_ptr submit_failure_;
};

} // namespace Internal
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
