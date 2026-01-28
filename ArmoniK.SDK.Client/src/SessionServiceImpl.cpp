#include "SessionServiceImpl.h"
#include "Batcher.h"
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

  std::vector<std::string> task_ids;
  task_ids.reserve(task_requests.size());

  std::mutex task_ids_mutex;

  Batcher<armonik::api::common::TaskCreation> submit_batcher(
      submit_batch_size_, [&](std::vector<armonik::api::common::TaskCreation> &&batch) {
        auto reply = channel_pool.WithChannel([&](auto channel) {
          return armonik::api::client::TasksClient(armonik::api::grpc::v1::tasks::Tasks::NewStub(channel))
              .submit_tasks(session, batch, static_cast<armonik::api::grpc::v1::TaskOptions>(task_options));
        });

        {
          std::lock_guard<std::mutex> lock(task_ids_mutex);
          for (auto &&t : reply) {
            task_ids.emplace_back(std::move(t.task_id));
          }
        }

        for (auto &&t : reply) {
          taskId_resultId[t.task_id] = t.expected_output_ids[0];
          resultId_taskId[t.expected_output_ids[0]] = t.task_id;
          result_handlers[t.expected_output_ids[0]] = handler;
        }
      });

  for (const auto &task_request : task_requests) {
    armonik::api::grpc::v1::TaskRequest request;
    // Serialize the request in an ArmoniK format
    *request.mutable_payload() = task_request.Serialize();
    // Set the data dependencies
    request.mutable_data_dependencies()->Add(task_request.data_dependencies.begin(),
                                             task_request.data_dependencies.end());

    armonik::api::common::TaskCreation creation{};

    auto result_payload = channel_pool.WithChannel([&](auto channel) {
      armonik::api::client::ResultsClient client(armonik::api::grpc::v1::results::Results::NewStub(channel));

      const std::size_t max_inline_bytes = client.get_service_configuration().data_chunk_max_size;

      const auto &payload_data = request.payload();
      std::vector<std::string> result_names = {"result"};

      // If we need to upload via stream, we create an extra result for the payload.
      // It is not required with small payloads as create_results creates the metadata for us.
      if (payload_data.size() > max_inline_bytes) {
        assert(task_request.method_name != "result");
        result_names.push_back(task_request.method_name);
      }

      auto results = client.create_results_metadata(session, result_names);
      auto result_id = results.at("result");

      std::string payload_id;

      std::stringstream msg;
      msg << "Preparing payload for method '" << task_request.method_name << "' (" << payload_data.size()
          << " bytes, inline limit " << max_inline_bytes << " bytes)";
      logger_.debug(msg.str());

      try {
        if (payload_data.size() <= max_inline_bytes) {
          //  small payload embeded directly as part of metadata
          logger_.debug("Using inline create_results for small payload.");

          payload_id =
              client
                  .create_results(session, std::vector<std::pair<std::string, std::string>>{{task_request.method_name,
                                                                                             request.payload()}})
                  .at(task_request.method_name);
        } else {
          // Streaming for large payload
          logger_.debug("Payload exceeds inline limit, using streaming upload.");

          payload_id = results.at(task_request.method_name);
          client.upload_result_data(session, payload_id, payload_data);
        }
      } catch (const armonik::api::common::exceptions::ArmoniKApiException &ex) {
        std::stringstream err;
        err << "Error uploading payload for '" << task_request.method_name << "': " << ex.what();
        logger_.error(err.str());
        throw;
      }

      return std::pair<std::string, std::string>{result_id, payload_id};
    });

    // Set payload ID
    creation.payload_id = std::move(result_payload.second);
    // One result per task
    creation.expected_output_keys.push_back(std::move(result_payload.first));
    creation.data_dependencies.insert(creation.data_dependencies.end(), task_request.data_dependencies.begin(),
                                      task_request.data_dependencies.end());

    // Add to batch, if number of requests in the batch attains the submitt_batch_size_ then
    // the Batcher will process them
    submit_batcher.Add(std::move(creation));
  }

  // Manually flush remaining requests before returning
  submit_batcher.ProcessBatch();

  return task_ids;
}

SessionServiceImpl::SessionServiceImpl(const Common::Properties &properties,
                                       armonik::api::common::logger::Logger &logger, const std::string &session_id)
    : taskOptions(properties.taskOptions), channel_pool(properties, logger), logger_(logger.local()),
      wait_batch_size_(properties.configuration.get_control_plane().getWaitBatchSize()),
      submit_batch_size_(properties.configuration.get_control_plane().getSubmitBatchSize()) {
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

  std::map<std::string, armonik::api::grpc::v1::results::ResultRaw> results;

  {
    std::lock_guard<std::mutex> _(maps_mutex);

    if (task_ids.empty()) {
      // If the task set is empty, wait for all the results that have been submitted at the moment of the wait
      for (auto &handler : result_handlers) {
        armonik::api::grpc::v1::results::ResultRaw result{};
        result.set_result_id(handler.first);
        result.set_status(armonik::api::grpc::v1::result_status::RESULT_STATUS_NOTFOUND);
        results.emplace(handler.first, std::move(result));
      }
    } else {
      // Otherwise, wait for the results of the specified tasks only
      for (auto &tid : task_ids) {
        auto result_id = taskId_resultId.find(tid);
        if (result_id == taskId_resultId.end()) {
          logger_.warning("Task ID " + tid + " has no associated result ID, skipping wait.");
          continue;
        }

        armonik::api::grpc::v1::results::ResultRaw result{};
        result.set_result_id(result_id->second);
        result.set_status(armonik::api::grpc::v1::result_status::RESULT_STATUS_NOTFOUND);
        results.emplace(result_id->second, std::move(result));
      }
    }
  }

  // Batcher to get results in batches
  Batcher<std::string> batcher(wait_batch_size_, [&](std::vector<std::string> &&batch) {
    armonik::api::grpc::v1::results::Filters filters{};
    for (auto &result_id : batch) {

      auto filter = filters.add_or_()->add_and_();
      filter->mutable_field()->mutable_result_raw_field()->set_field(
          armonik::api::grpc::v1::results::RESULT_RAW_ENUM_FIELD_RESULT_ID);
      filter->mutable_filter_string()->set_value(std::move(result_id));
      filter->mutable_filter_string()->set_operator_(armonik::api::grpc::v1::FILTER_STRING_OPERATOR_EQUAL);
    }

    auto response = channel_pool.WithChannel([&](auto &&channel) {
      armonik::api::client::ResultsClient resultsClient(armonik::api::grpc::v1::results::Results::NewStub(channel));
      int total = 0;
      return resultsClient.list_results(std::move(filters), total, 0, filters.or__size());
    });

    for (auto &result : response) {
      // threadsafe as the result is known to be present in the map and we have unique keys
      results[result.result_id()] = std::move(result);
    }
  });

  // Wait all the specified results
  while (!results.empty()) {
    bool hasError = false;

    // Get all the results using batched requests
    for (auto &result : results) {
      batcher.Add(result.first);
    }
    batcher.ProcessBatch();

    for (auto result_it = results.begin(); result_it != results.end();) {
      auto &result = result_it->second;
      auto status = result.status();

      // reset status to not found for next iteration if not completed or aborted
      result.set_status(armonik::api::grpc::v1::result_status::RESULT_STATUS_NOTFOUND);

      std::shared_ptr<IServiceInvocationHandler> handler{};
      std::string task_id{};

      // Extract the handler and taskid information
      if (status != armonik::api::grpc::v1::result_status::RESULT_STATUS_CREATED) {
        std::lock_guard<std::mutex> _(maps_mutex);
        auto handler_it = result_handlers.find(result.result_id());
        if (handler_it != result_handlers.end()) {
          handler = std::move(handler_it->second);
          result_handlers.erase(handler_it);
        }
        auto task_id_it = resultId_taskId.find(result.result_id());
        if (task_id_it != resultId_taskId.end()) {
          task_id = std::move(task_id_it->second);
          resultId_taskId.erase(task_id_it);
          taskId_resultId.erase(task_id);
        }
      }

      // function to be called upon errors
      auto handle_error = [&](const std::exception &e, const std::string &reason = {}) {
        hasError = true;
        std::stringstream message;
        message << "Error while handling result " << result.result_id() << " for task "
                << (task_id.empty() ? "[UNKNOWN]" : task_id);
        if (!reason.empty()) {
          message << " : " << reason;
        }
        message << " : " << e.what();
        logger_.error(message.str());
        if (handler) {
          handler->HandleError(e, task_id);
        }
      };

      std::string payload, owner_task_id;
      switch (status) {
      // If the result is created, we should wait for the next turn
      case armonik::api::grpc::v1::result_status::RESULT_STATUS_CREATED:
        ++result_it;
        continue;

      // If the result is completed, we download it
      case armonik::api::grpc::v1::result_status::RESULT_STATUS_COMPLETED:
        // Download the payload
        try {
          payload = channel_pool.WithChannel([&](auto &&channel) {
            return armonik::api::client::ResultsClient(armonik::api::grpc::v1::results::Results::NewStub(channel))
                .download_result_data(session, result.result_id());
          });
        } catch (const std::exception &e) {
          handle_error(e, "Failed to download result data");
        }

        // Call the response handler with the payload
        try {
          handler->HandleResponse(payload, task_id);
        } catch (const std::exception &e) {
          handle_error(e, "Failed to execute result handler");
        }
        break;

      // If the result is aborted, we retrieve the task error
      case armonik::api::grpc::v1::result_status::RESULT_STATUS_ABORTED:
        // The considered task is either the owner task of the result, or the original task
        owner_task_id = result.owner_task_id();
        if (result.owner_task_id().empty()) {
          owner_task_id = task_id;
        }

        if (owner_task_id.empty()) {
          handle_error(armonik::api::common::exceptions::ArmoniKApiException("Result is aborted"));
        } else {
          armonik::api::grpc::v1::TaskError error{};
          error.set_task_id(owner_task_id);

          // Retrieve the task error details
          try {
            channel_pool.WithChannel([&](auto &&channel) {
              auto task = armonik::api::client::TasksClient(armonik::api::grpc::v1::tasks::Tasks::NewStub(channel))
                              .get_task(owner_task_id);
              auto task_error = error.add_errors();
              task_error->set_detail(std::move(task.output().error()));
              task_error->set_task_status(task.status());
            });
          } catch (const std::exception &e) {
            error.add_errors()->set_detail(e.what());
          }
          handle_error(armonik::api::common::exceptions::ArmoniKTaskError("Result is aborted", error));
        }
        break;

      // In all other cases, we just call the handler on a generic error
      case armonik::api::grpc::v1::result_status::RESULT_STATUS_DELETED:
        handle_error(armonik::api::common::exceptions::ArmoniKApiException("Result is deleted"));
        break;
      case armonik::api::grpc::v1::result_status::RESULT_STATUS_NOTFOUND:
        handle_error(armonik::api::common::exceptions::ArmoniKApiException("Result was not found"));
        break;
      default:
        handle_error(armonik::api::common::exceptions::ArmoniKApiException("Result status is unknown"));
        break;
      }

      result_it = results.erase(result_it);
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

void SessionServiceImpl::CloseSession() {
  auto reply = channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
    return armonik::api::client::SessionsClient(armonik::api::grpc::v1::sessions::Sessions::NewStub(channel))
        .close_session(session);
  });
}

void SessionServiceImpl::CancelSession() {
  auto reply = channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
    return armonik::api::client::SessionsClient(armonik::api::grpc::v1::sessions::Sessions::NewStub(channel))
        .cancel_session(session);
  });
}

void SessionServiceImpl::PurgeSession() {
  auto reply = channel_pool.WithChannel([&](const std::shared_ptr<::grpc::Channel> &channel) {
    return armonik::api::client::SessionsClient(armonik::api::grpc::v1::sessions::Sessions::NewStub(channel))
        .purge_session(session);
  });
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
        logger_.info(std::string("Couldn't completely destroy batch of results : ") + e.what());
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
