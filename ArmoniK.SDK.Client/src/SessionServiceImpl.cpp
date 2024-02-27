#include "SessionServiceImpl.h"
#include "armonik/sdk/client/IServiceInvocationHandler.h"
#include <armonik/client/results/ResultsClient.h>
#include <armonik/client/results_common.pb.h>
#include <armonik/client/results_service.grpc.pb.h>
#include <armonik/client/sessions/SessionsClient.h>
#include <armonik/client/sessions_service.grpc.pb.h>
#include <armonik/client/tasks/TasksClient.h>
#include <armonik/client/tasks_service.grpc.pb.h>
#include <armonik/common/exceptions/ArmoniKApiException.h>
#include <armonik/common/exceptions/ArmoniKTaskError.h>
#include <armonik/common/objects.pb.h>
#include <armonik/common/utils/GuuId.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

const std::string &SessionServiceImpl::getSession() const { return session; }

[[maybe_unused]] std::vector<std::string>
SessionServiceImpl::Submit(const std::vector<Common::TaskPayload> &task_requests,
                           std::shared_ptr<IServiceInvocationHandler> handler,
                           const Common::TaskOptions &task_options) {

  std::vector<armonik::api::common::TaskCreation> task_creations;
  task_creations.reserve(task_requests.size());

  for (const auto &task_request : task_requests) {
    armonik::api::grpc::v1::TaskRequest request;
    // Serialize the request in an ArmoniK format
    *request.mutable_payload() = task_request.Serialize();
    // Set the data dependencies
    request.mutable_data_dependencies()->Add(task_request.data_dependencies.begin(),
                                             task_request.data_dependencies.end());

    armonik::api::common::TaskCreation creation{};

    auto result_payload = channel_pool.WithChannel([&](auto channel) {
      auto client = armonik::api::client::ResultsClient(armonik::api::grpc::v1::results::Results::NewStub(channel));
      auto result = client.create_results_metadata(session, std::vector<std::string>{"result"})["result"];
      auto payload =
          client.create_results(session, std::vector<std::pair<std::string, std::string>>{
                                             {task_request.method_name, request.payload()}})[task_request.method_name];

      return std::pair<std::string, std::string>{result, payload};
    });

    // Set payload ID
    creation.payload_id = std::move(result_payload.second);
    // One result per task
    creation.expected_output_keys.push_back(std::move(result_payload.first));
    creation.data_dependencies.insert(creation.data_dependencies.end(), task_request.data_dependencies.begin(),
                                      task_request.data_dependencies.end());

    task_creations.emplace_back(std::move(creation));
  }

  auto reply = channel_pool.WithChannel([&](auto channel) {
    return armonik::api::client::TasksClient(armonik::api::grpc::v1::tasks::Tasks::NewStub(channel))
        .submit_tasks(session, task_creations, static_cast<armonik::api::grpc::v1::TaskOptions>(task_options));
  });

  std::stringstream message;
  message << "Task ID " << reply[0].task_id << std::endl;
  logger_.log(armonik::api::common::logger::Level::Error, message.str());

  std::vector<std::string> task_ids;
  task_ids.reserve(task_requests.size());
  {
    std::lock_guard<std::mutex> lock(maps_mutex);
    for (auto &&t : reply) {
      task_ids.emplace_back(std::move(t.task_id));
    }
    tid_rids = channel_pool.WithChannel([&](auto channel) {
      return armonik::api::client::TasksClient(armonik::api::grpc::v1::tasks::Tasks::NewStub(channel))
          .get_result_ids(task_ids);
    });
    for (auto &&tid_rid : tid_rids) {
      taskId_resultId[task_ids[0]] = tid_rid.second[0];
      resultId_taskId[tid_rid.second[0]] = tid_rid.first;
      result_handlers[tid_rid.second[0]] = handler;
    }
  }
  return task_ids;
}

SessionServiceImpl::SessionServiceImpl(const Common::Properties &properties,
                                       armonik::api::common::logger::Logger &logger, const std::string &session_id)
    : taskOptions(properties.taskOptions), channel_pool(properties, logger), logger_(logger.local()) {
  // Creates a new session
  session = session_id.empty() ? channel_pool.WithChannel([&](auto &&channel) {
    return armonik::api::client::SessionsClient(armonik::api::grpc::v1::sessions::Sessions::NewStub(channel))
        .create_session(static_cast<armonik::api::grpc::v1::TaskOptions>(properties.taskOptions),
                        {properties.taskOptions.partition_id});
  })
                               : session_id;
}

std::vector<std::string> SessionServiceImpl::Submit(const std::vector<Common::TaskPayload> &task_requests,
                                                    std::shared_ptr<IServiceInvocationHandler> handler) {
  return Submit(task_requests, std::move(handler), taskOptions);
}

void SessionServiceImpl::WaitResults(std::set<std::string> task_ids, WaitBehavior behavior,
                                     const WaitOptions &options) {
  auto function_stop = std::chrono::steady_clock::now() + std::chrono::milliseconds(options.timeout);

  bool hasWaitList = !task_ids.empty();
  size_t initialTaskIds_size = task_ids.size();
  bool breakOnError = behavior & WaitBehavior::BreakOnError;
  bool stopOnFirst = behavior & WaitBehavior::Any;

  // While either we wait on all tasks, or on the tasks given
  while (!hasWaitList || !task_ids.empty()) {
    std::vector<std::string> resultIds;

    {
      std::lock_guard<std::mutex> _(maps_mutex);
      resultIds.reserve(resultId_taskId.size());
      for (auto &&rid_tid : resultId_taskId) {
        resultIds.push_back(rid_tid.first);
      }
    }

    // If there is no task to wait on, return
    if (resultIds.empty()) {
      break;
    }

    bool hasError = false;
    // Ask for the results statuses
    auto statuses = channel_pool.WithChannel([&](auto &&channel) {
      std::vector<armonik::api::grpc::v1::results::ResultRaw> raws;
      raws.reserve(resultIds.size());
      std::transform(resultIds.cbegin(), resultIds.cend(), std::back_inserter(raws), [channel](auto &&resultId) {
        return armonik::api::client::ResultsClient(armonik::api::grpc::v1::results::Results::NewStub(channel))
            .get_result(resultId);
      });
      return raws;
    });

    std::vector<std::pair<std::string, std::string>> done;

    // For each status
    for (auto &&result_raw : statuses) {
      std::shared_ptr<IServiceInvocationHandler> handler;
      std::string task_id;
      if (result_raw.status() == armonik::api::grpc::v1::result_status::RESULT_STATUS_COMPLETED ||
          result_raw.status() == armonik::api::grpc::v1::result_status::RESULT_STATUS_ABORTED) {
        // Get the handler and taskid information
        {
          std::lock_guard<std::mutex> _(maps_mutex);
          handler = result_handlers.at(result_raw.result_id());
          task_id = resultId_taskId.at(result_raw.result_id());
        }

        armonik::api::grpc::v1::ResultRequest resultRequest;
        resultRequest.set_session(session);
        resultRequest.set_result_id(result_raw.result_id());

        try {
          // Get the finished result
          auto payload = channel_pool.WithChannel([&](auto &&channel) {
            return armonik::api::client::ResultsClient(armonik::api::grpc::v1::results::Results::NewStub(channel))
                .download_result_data(session, result_raw.result_id());
          });
          if (result_raw.status() == armonik::api::grpc::v1::result_status::RESULT_STATUS_ABORTED) {
            // The above command should have failed !
            throw armonik::api::common::exceptions::ArmoniKApiException(
                "Result is aborted and shouldn't have been retrieved");
          }

          // Handle the result
          handler->HandleResponse(payload, task_id);
        } catch (const armonik::api::common::exceptions::ArmoniKTaskError &e) {
          // Task is in error
          hasError |= task_ids.find(task_id) != task_ids.end();
          std::stringstream message;
          message << "Task " << task_id << " is in error : " << e.what() << std::endl;
          logger_.log(armonik::api::common::logger::Level::Error, message.str());
          handler->HandleError(e, task_id);

        } catch (const armonik::api::common::exceptions::ArmoniKApiException &e) {
          // Internal error
          hasError |= task_ids.find(task_id) != task_ids.end();
          std::stringstream message;
          message << "Internal error while handling result " << e.what() << std::endl;
          logger_.log(armonik::api::common::logger::Level::Error, message.str());
          handler->HandleError(e, task_id);
        }
        done.emplace_back(task_id, result_raw.result_id());
      } else if (result_raw.status() == armonik::api::grpc::v1::result_status::RESULT_STATUS_NOTFOUND) {
        std::stringstream message;
        message << "Result " << result_raw.result_id() << " not found" << std::endl;
        logger_.log(armonik::api::common::logger::Level::Info, message.str());
        {
          std::lock_guard<std::mutex> _(maps_mutex);
          if (resultId_taskId.find(result_raw.result_id()) != resultId_taskId.end()) {
            task_id = resultId_taskId.at(result_raw.result_id());
          }
        }
        done.emplace_back(task_id, result_raw.result_id());
      }
    }

    {
      // Remove all finished tasks and results from the global lists
      std::lock_guard<std::mutex> _(maps_mutex);
      for (auto &&tid_rid : done) {
        taskId_resultId.erase(tid_rid.first);
        resultId_taskId.erase(tid_rid.second);
        result_handlers.erase(tid_rid.second);
      }
    }

    // Remove all finished task_ids from the waiting list
    for (auto &&tid_rid : done) {
      task_ids.erase(tid_rid.first);
    }

    // If we wait for any and at least one is done, or if we break on error and had an error, then return
    if ((stopOnFirst && task_ids.size() < initialTaskIds_size) || (breakOnError && hasError)) {
      break;
    }
    if (std::chrono::steady_clock::now() > function_stop) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(options.polling_ms));
  }
}

void SessionServiceImpl::DropSession() {
  // Clear all the maps
  {
    std::lock_guard<std::mutex> _(maps_mutex);
    taskId_resultId.clear();
    resultId_taskId.clear();
    result_handlers.clear();
  }
  // Cancel the session
  auto reply = channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
    return armonik::api::client::SessionsClient(armonik::api::grpc::v1::sessions::Sessions::NewStub(channel))
        .cancel_session(session);
  });

  // Create the result filter for result.session_id == session
  armonik::api::grpc::v1::results::Filters filters;
  armonik::api::grpc::v1::results::FilterField filter_field;
  filter_field.mutable_field()->mutable_result_raw_field()->set_field(
      armonik::api::grpc::v1::results::RESULT_RAW_ENUM_FIELD_SESSION_ID);
  filter_field.mutable_filter_string()->set_value(session);
  filter_field.mutable_filter_string()->set_operator_(armonik::api::grpc::v1::FILTER_STRING_OPERATOR_EQUAL);
  *filters.mutable_or_()->Add()->mutable_and_()->Add() = filter_field;
  int page = 0;
  const int page_size = 500;
  int total = 0;
  do {
    channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
      auto results = armonik::api::client::ResultsClient(armonik::api::grpc::v1::results::Results::NewStub(channel));
      // List results
      auto rawList = results.list_results(filters, total, page++, page_size);
      std::vector<std::string> ids;
      ids.reserve(rawList.size());
      for (auto &&raw : rawList) {
        ids.push_back(raw.result_id());
      }
      // Delete results
      try {
        results.delete_results_data(session, ids);
      } catch (const std::exception &e) {
        logger_.log(armonik::api::common::logger::Level::Info,
                    std::string("Couldn't completely destroy batch of results : ") + e.what());
      }
    });
  } while (page * page_size < total);
}

void SessionServiceImpl::CleanupTasks(std::vector<std::string> task_ids) {
  // Remove the given tasks from the maps
  {
    std::lock_guard<std::mutex> _(maps_mutex);
    for (auto &&t : task_ids) {
      auto loc = taskId_resultId.find(t);
      if (loc != taskId_resultId.end()) {
        resultId_taskId.erase(loc->second);
        result_handlers.erase(loc->second);
        taskId_resultId.erase(loc->first);
      }
    }
  }
  const size_t batch_size = 500;
  auto tasks_iterator = task_ids.begin();
  // Cancel the given tasks
  while (tasks_iterator != task_ids.end()) {
    channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
      std::vector<std::string> batched_ids;
      for (size_t i = 0; i < batch_size && tasks_iterator != task_ids.end(); ++i) {
        batched_ids.push_back(*tasks_iterator);
        tasks_iterator++;
      }
      armonik::api::client::TasksClient(armonik::api::grpc::v1::tasks::Tasks::NewStub(channel))
          .cancel_tasks(batched_ids);
    });
  }

  tasks_iterator = task_ids.begin();

  while (tasks_iterator != task_ids.end()) {
    // List batch of results from the given tasks
    auto map_results = channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
      std::vector<std::string> batched_ids;
      for (size_t i = 0; i < batch_size && tasks_iterator != task_ids.end(); ++i) {
        batched_ids.push_back(std::move(*tasks_iterator));
        tasks_iterator++;
      }
      return armonik::api::client::TasksClient(armonik::api::grpc::v1::tasks::Tasks::NewStub(channel))
          .get_result_ids(batched_ids);
    });

    // Delete results
    channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
      auto results = armonik::api::client::ResultsClient(armonik::api::grpc::v1::results::Results::NewStub(channel));
      std::vector<std::string> resultIds;
      resultIds.reserve(map_results.size());
      for (auto &&taskId_resultIds : map_results) {
        resultIds.insert(resultIds.end(), std::make_move_iterator(taskId_resultIds.second.begin()),
                         std::make_move_iterator(taskId_resultIds.second.end()));
      }
      results.delete_results_data(session, resultIds);
    });
  }
}

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
