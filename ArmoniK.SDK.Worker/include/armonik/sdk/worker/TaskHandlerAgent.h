#pragma once

#include "TaskContext.h"
#include <algorithm>
#include <armonik/common/utils/string_view.h>
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/worker/Worker/TaskHandler.h>
#include <future>
#include <string>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Worker {

/**
 * @brief TaskAgent implemented with the TaskHandler of ArmoniK.Api.Worker
 *
 * @note Header-only, like ConventionExecutor: it requires ArmoniK.Api.Worker and ArmoniK.SDK.Common.
 */
class TaskHandlerAgent final : public TaskAgent {
public:
  /**
   * @brief Creates the agent of a task
   * @param taskHandler Task handler. Must outlive the agent.
   */
  explicit TaskHandlerAgent(armonik::api::worker::TaskHandler &taskHandler)
      : taskHandler(taskHandler), options(taskHandler.getTaskOptions()) {}

  const std::string &session_id() const override { return taskHandler.getSessionId(); }
  const std::string &task_id() const override { return taskHandler.getTaskId(); }
  const Common::TaskOptions &task_options() const override { return options; }

  std::vector<std::string> create_results_metadata(const std::vector<std::string> &names) override {
    std::vector<armonik::api::grpc::v1::agent::CreateResultsMetaDataRequest_ResultCreate> results(names.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
      results[i].set_name(names[i]);
    }
    return taskHandler.get_result_ids(std::move(results));
  }

  void send_results(const std::vector<ResultData> &results) override {
    // Each send_result runs on its own thread: bound how many run at once
    const std::size_t max_in_flight = 16;
    for (std::size_t begin = 0; begin < results.size(); begin += max_in_flight) {
      const auto end = std::min(results.size(), begin + max_in_flight);
      std::vector<std::future<void>> sends;
      for (auto i = begin; i < end; ++i) {
        sends.push_back(
            taskHandler.send_result(results[i].id, armonik::api::string_view(results[i].data, results[i].size)));
      }
      for (auto &send : sends) {
        send.get();
      }
    }
  }

  std::vector<std::string> submit_tasks(const Common::TaskOptions &task_options,
                                        const std::vector<TaskRequest> &tasks) override {
    std::vector<armonik::api::grpc::v1::TaskRequest> requests(tasks.size());
    for (std::size_t i = 0; i < tasks.size(); ++i) {
      requests[i].set_payload(tasks[i].payload);
      for (const auto &id : tasks[i].data_dependencies) {
        requests[i].add_data_dependencies(id);
      }
      for (const auto &id : tasks[i].expected_outputs) {
        requests[i].add_expected_output_keys(id);
      }
    }
    // get() in the same expression: the asynchronous call refers to its by-value options argument
    const auto reply =
        taskHandler.create_tasks_async(static_cast<armonik::api::grpc::v1::TaskOptions>(task_options), requests)
            .get();
    // A rejection by the agent fails the task: a retry would submit the same request again
    if (reply.Response_case() != armonik::api::grpc::v1::agent::CreateTaskReply::kCreationStatusList) {
      throw Common::ArmoniKSdkException("Failed to submit subtasks: " + reply.error());
    }
    std::vector<std::string> task_ids;
    for (const auto &status : reply.creation_status_list().creation_statuses()) {
      if (status.Status_case() != armonik::api::grpc::v1::agent::CreateTaskReply::CreationStatus::kTaskInfo) {
        throw Common::ArmoniKSdkException("Failed to submit a subtask: " + status.error());
      }
      task_ids.push_back(status.task_info().task_id());
    }
    return task_ids;
  }

private:
  armonik::api::worker::TaskHandler &taskHandler;
  Common::TaskOptions options;
};

} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
