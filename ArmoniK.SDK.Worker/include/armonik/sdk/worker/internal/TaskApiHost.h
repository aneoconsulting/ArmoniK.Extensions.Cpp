#pragma once

#include "../ArmoniKSDKInterface.h"
#include "../TaskContext.h"
#include "TaskOptionsJson.h"
#include <algorithm>
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <cstring>
#include <exception>
#include <map>
#include <string>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Worker {
namespace Internal {

/**
 * @brief Worker side of armonik_process: exposes a task and its TaskAgent to a library as an armonik_task_t
 *
 * It tracks the outputs of the task, so that the worker checks them whatever armonik_process the library defines:
 * each output is written once or delegated to a subtask, not both.
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
      output_states_.emplace(kv.second, Output{kv.first, OutputState::Pending});
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
   * @brief Checks that every output was written or delegated. Called by the worker after armonik_process succeeds.
   * @throws ArmoniKSdkException if an output was neither written nor delegated
   */
  void finish() const {
    std::string pending;
    for (const auto &kv : output_states_) {
      if (kv.second.state == OutputState::Pending) {
        pending += (pending.empty() ? "'" : ", '") + kv.second.name + "'";
      }
    }
    if (!pending.empty()) {
      throw Common::ArmoniKSdkException("Outputs neither written nor delegated to a subtask: " + pending);
    }
  }

private:
  enum class OutputState { Pending, Written, Delegated };
  struct Output {
    std::string name;
    OutputState state;
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
   * @brief Checks that an output of the task can still be written or delegated, if the id is an output of the task
   * @return The output, or nullptr if the id is not an output of the task
   */
  Output *AvailableOutput(const std::string &id) {
    const auto it = output_states_.find(id);
    if (it == output_states_.end()) {
      return nullptr;
    }
    if (it->second.state == OutputState::Written) {
      throw Common::ArmoniKSdkException("Output '" + it->second.name + "' is already written");
    }
    if (it->second.state == OutputState::Delegated) {
      throw Common::ArmoniKSdkException("Output '" + it->second.name + "' is delegated to a subtask");
    }
    return &it->second;
  }

  static armonik_status_t CreateResultsMetadata(void *host, const armonik_string_t *names, size_t count,
                                                void *sink_context, armonik_string_sink_t on_value,
                                                armonik_string_sink_t on_error) {
    return Guard(sink_context, on_error, [&]() {
      std::vector<std::string> result_names;
      for (size_t i = 0; i < count; ++i) {
        result_names.emplace_back(names[i].data, names[i].size);
      }
      for (const auto &id : static_cast<TaskApiHost *>(host)->agent_.create_results_metadata(result_names)) {
        on_value(sink_context, id.data(), id.size());
      }
    });
  }

  static armonik_status_t SendResults(void *host, const armonik_result_data_t *results, size_t count,
                                      void *sink_context, armonik_string_sink_t on_error) {
    return Guard(sink_context, on_error, [&]() {
      auto &self = *static_cast<TaskApiHost *>(host);
      std::vector<TaskAgent::ResultData> data;
      std::vector<Output *> written;
      for (size_t i = 0; i < count; ++i) {
        data.push_back(
            {std::string(results[i].id.data, results[i].id.size), results[i].data.data, results[i].data.size});
        if (const auto output = self.AvailableOutput(data.back().id)) {
          if (std::find(written.begin(), written.end(), output) != written.end()) {
            throw Common::ArmoniKSdkException("Output '" + output->name + "' is written twice");
          }
          written.push_back(output);
        }
      }
      self.agent_.send_results(data);
      for (const auto output : written) {
        output->state = OutputState::Written;
      }
    });
  }

  static armonik_status_t SubmitTasks(void *host, armonik_string_t task_options, const armonik_task_request_t *tasks,
                                      size_t count, void *sink_context, armonik_string_sink_t on_value,
                                      armonik_string_sink_t on_error) {
    return Guard(sink_context, on_error, [&]() {
      auto &self = *static_cast<TaskApiHost *>(host);
      const auto options = TaskOptionsFromJson(task_options.data, task_options.size);
      std::vector<Output *> delegated;
      std::vector<TaskAgent::TaskRequest> requests(count);
      for (size_t i = 0; i < count; ++i) {
        requests[i].payload.assign(tasks[i].payload.data, tasks[i].payload.size);
        for (size_t j = 0; j < tasks[i].data_dependencies_count; ++j) {
          requests[i].data_dependencies.emplace_back(tasks[i].data_dependencies[j].data,
                                                     tasks[i].data_dependencies[j].size);
        }
        for (size_t j = 0; j < tasks[i].expected_outputs_count; ++j) {
          requests[i].expected_outputs.emplace_back(tasks[i].expected_outputs[j].data,
                                                    tasks[i].expected_outputs[j].size);
          if (const auto output = self.AvailableOutput(requests[i].expected_outputs.back())) {
            if (std::find(delegated.begin(), delegated.end(), output) != delegated.end()) {
              throw Common::ArmoniKSdkException("Output '" + output->name + "' is delegated to two subtasks");
            }
            delegated.push_back(output);
          }
        }
      }
      // Delegated before submitting: subtasks created before a rejection may still produce the outputs
      for (const auto output : delegated) {
        output->state = OutputState::Delegated;
      }
      for (const auto &id : self.agent_.submit_tasks(options, requests)) {
        on_value(sink_context, id.data(), id.size());
      }
    });
  }

  TaskAgent &agent_;
  std::string options_;
  std::map<std::string, Output> output_states_; // blob id → output
  std::vector<armonik_input_t> inputs_;
  std::vector<armonik_output_t> outputs_;
  armonik_task_t task_{};
};

} // namespace Internal
} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
