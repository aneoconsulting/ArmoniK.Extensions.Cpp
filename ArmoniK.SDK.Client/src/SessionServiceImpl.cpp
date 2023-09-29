#include "SessionServiceImpl.h"
#include "armonik/sdk/client/IServiceInvocationHandler.h"
#include <armonik/client/results_common.pb.h>
#include <armonik/common/exceptions/ArmoniKApiException.h>
#include <armonik/common/exceptions/ArmoniKTaskError.h>
#include <armonik/common/objects.pb.h>
#include <armonik/common/utils/GuuId.h>
#include <armonik/sdk/common/Properties.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <grpcpp/client_context.h>
#include <submitter_service.grpc.pb.h>
#include <thread>
#include <utility>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {

std::vector<std::string> SessionServiceImpl::generate_result_ids(size_t num) {
  armonik::api::grpc::v1::results::CreateResultsMetaDataRequest results_request;
  armonik::api::grpc::v1::results::CreateResultsMetaDataResponse results_response;

  // Creates the result creation requests
  std::vector<armonik::api::grpc::v1::results::CreateResultsMetaDataRequest_ResultCreate> results_create;
  results_create.reserve(num);
  for (size_t i = 0; i < num; i++) {
    armonik::api::grpc::v1::results::CreateResultsMetaDataRequest_ResultCreate result_create;
    // Random name
    *result_create.mutable_name() = armonik::api::common::utils::GuuId::generate_uuid();
    results_create.push_back(std::move(result_create));
  }

  results_request.mutable_results()->Add(std::make_move_iterator(results_create.begin()),
                                         std::make_move_iterator(results_create.end()));
  *results_request.mutable_session_id() = session;

  // Creates the results
  auto status = channel_pool.WithChannel([&](auto &&channel) {
    grpc::ClientContext context;
    return armonik::api::grpc::v1::results::Results::NewStub(channel)->CreateResultsMetaData(&context, results_request,
                                                                                             &results_response);
  });

  if (!status.ok()) {
    std::stringstream message;
    message << "Error: " << status.error_code() << ": " << status.error_message()
            << ". details : " << status.error_details() << std::endl;
    logger_.log(armonik::api::common::logger::Level::Error, "Could not create results for submit: ");
    throw armonik::api::common::exceptions::ArmoniKApiException(message.str());
  }
  std::vector<std::string> result_ids;
  result_ids.reserve(num);
  // Get the result ids from the response
  std::transform(results_response.mutable_results()->begin(), results_response.mutable_results()->end(),
                 std::back_inserter(result_ids), [](auto &res) { return std::move(*res.mutable_result_id()); });
  return result_ids;
}

const std::string &SessionServiceImpl::getSession() const { return session; }

[[maybe_unused]] std::vector<std::string>
SessionServiceImpl::Submit(const std::vector<Common::TaskPayload> &task_requests,
                           std::shared_ptr<IServiceInvocationHandler> handler,
                           const Common::TaskOptions &task_options) {
  std::vector<armonik::api::grpc::v1::TaskRequest> definitions;
  definitions.reserve(task_requests.size());

  // Creates the needed result ids
  auto result_ids = generate_result_ids(task_requests.size());

  for (size_t i = 0; i < task_requests.size(); i++) {
    armonik::api::grpc::v1::TaskRequest request;
    // Serialize the request in an ArmoniK format
    *request.mutable_payload() = task_requests[i].Serialize();
    // One result per task
    request.add_expected_output_keys(std::move(result_ids[i]));
    // Set the data dependencies
    request.mutable_data_dependencies()->Add(task_requests[i].data_dependencies.begin(),
                                             task_requests[i].data_dependencies.end());

    definitions.push_back(std::move(request));
  }
  auto reply = channel_pool.WithChannel([&](auto channel) {
    return armonik::api::client::SubmitterClient(armonik::api::grpc::v1::submitter::Submitter::NewStub(channel))
        .create_tasks_async(session, static_cast<armonik::api::grpc::v1::TaskOptions>(task_options), definitions)
        .get();
  });

  auto &list = *reply.mutable_creation_status_list()->mutable_creation_statuses();
  std::vector<std::string> task_ids;
  task_ids.reserve(task_requests.size());
  {
    std::lock_guard<std::mutex> lock(maps_mutex);
    for (auto &&t : list) {
      task_ids.push_back(t.task_info().task_id());
      taskId_resultId[t.task_info().task_id()] = t.task_info().expected_output_keys(0);
      resultId_taskId[t.task_info().expected_output_keys(0)] = std::move(*t.mutable_task_info()->mutable_task_id());
      result_handlers[t.task_info().expected_output_keys(0)] = handler;
    }
  }
  return task_ids;
}

SessionServiceImpl::SessionServiceImpl(const Common::Properties &properties,
                                       armonik::api::common::logger::Logger &logger)
    : taskOptions(properties.taskOptions), channel_pool(properties, logger), logger_(logger.local()) {

  // Creates a new session
  session = channel_pool.WithChannel([&](auto &&channel) {
    return armonik::api::client::SubmitterClient(armonik::api::grpc::v1::submitter::Submitter::NewStub(channel))
        .create_session(static_cast<armonik::api::grpc::v1::TaskOptions>(properties.taskOptions),
                        {properties.taskOptions.partition_id});
  });
}

std::vector<std::string> SessionServiceImpl::Submit(const std::vector<Common::TaskPayload> &task_requests,
                                                    std::shared_ptr<IServiceInvocationHandler> handler) {
  return Submit(task_requests, std::move(handler), taskOptions);
}

void SessionServiceImpl::WaitResults(std::set<std::string> task_ids, WaitBehavior behavior,
                                     const WaitOptions &options) {

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
      return armonik::api::client::SubmitterClient(armonik::api::grpc::v1::submitter::Submitter::NewStub(channel))
          .get_result_status(session, resultIds);
    });

    std::vector<std::pair<std::string, std::string>> done;

    // For each status
    for (auto &&rid_status : statuses) {
      std::shared_ptr<IServiceInvocationHandler> handler;
      std::string task_id;
      if (rid_status.second == armonik::api::grpc::v1::result_status::RESULT_STATUS_COMPLETED ||
          rid_status.second == armonik::api::grpc::v1::result_status::RESULT_STATUS_ABORTED) {
        // Get the handler and taskid information
        {
          std::lock_guard<std::mutex> _(maps_mutex);
          handler = result_handlers.at(rid_status.first);
          task_id = resultId_taskId.at(rid_status.first);
        }

        armonik::api::grpc::v1::ResultRequest resultRequest;
        resultRequest.set_session(session);
        resultRequest.set_result_id(rid_status.first);

        try {
          // Get the finished result
          auto payload = channel_pool.WithChannel([&](auto &&channel) {
            return armonik::api::client::SubmitterClient(armonik::api::grpc::v1::submitter::Submitter::NewStub(channel))
                .get_result_async(resultRequest)
                .get();
          });
          if (rid_status.second == armonik::api::grpc::v1::result_status::RESULT_STATUS_ABORTED) {
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
        done.emplace_back(task_id, rid_status.first);
      } else if (rid_status.second == armonik::api::grpc::v1::result_status::RESULT_STATUS_NOTFOUND) {
        std::stringstream message;
        message << "Result " << rid_status.first << " not found" << std::endl;
        logger_.log(armonik::api::common::logger::Level::Info, message.str());
        done.emplace_back("", rid_status.first);
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

    std::this_thread::sleep_for(std::chrono::milliseconds(options.polling_ms));
  }
}

} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
