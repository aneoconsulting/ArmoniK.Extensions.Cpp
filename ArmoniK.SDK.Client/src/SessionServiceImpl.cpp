#include "SessionServiceImpl.h"
#include "IServiceInvocationHandler.h"
#include "Properties.h"
#include "TaskPayload.h"
#include <armonik/client/results_common.pb.h>
#include <armonik/common/exceptions/ArmoniKApiException.h>
#include <armonik/common/exceptions/ArmoniKTaskError.h>
#include <armonik/common/objects.pb.h>
#include <armonik/common/utils/GuuId.h>
#include <grpcpp/client_context.h>
#include <string>
#include <submitter_common.pb.h>
#include <submitter_service.grpc.pb.h>
#include <thread>
#include <utility>
#include <vector>

namespace SDK_CLIENT_NAMESPACE::Internal {

std::vector<std::string> SessionServiceImpl::generate_result_ids(size_t num) {
  grpc::ClientContext context;
  armonik::api::grpc::v1::results::CreateResultsMetaDataRequest results_request;
  armonik::api::grpc::v1::results::CreateResultsMetaDataResponse results_response;

  // Creates the result creation requests
  std::vector<armonik::api::grpc::v1::results::CreateResultsMetaDataRequest_ResultCreate> results_create;
  results_create.reserve(num);
  for (int i = 0; i < num; i++) {
    armonik::api::grpc::v1::results::CreateResultsMetaDataRequest_ResultCreate result_create;
    // Random name
    *result_create.mutable_name() = ArmoniK::Api::Common::utils::GuuId::generate_uuid();
    results_create.push_back(result_create);
  }

  results_request.mutable_results()->Add(results_create.begin(), results_create.end());
  *results_request.mutable_session_id() = session;

  // Creates the results
  auto status = results->CreateResultsMetaData(&context, results_request, &results_response);

  if (!status.ok()) {
    std::stringstream message;
    message << "Error: " << status.error_code() << ": " << status.error_message()
            << ". details : " << status.error_details() << std::endl;
    std::cerr << "Could not create results for submit: " << std::endl;
    throw ArmoniK::Api::Common::exceptions::ArmoniKApiException(message.str());
  }
  std::vector<std::string> result_ids;
  result_ids.reserve(num);
  // Get the result ids from the response
  std::transform(results_response.results().begin(), results_response.results().end(), std::back_inserter(result_ids),
                 [](auto res) { return res.result_id(); });
  return result_ids;
}

const std::string &SessionServiceImpl::getSession() const { return session; }

[[maybe_unused]] std::vector<std::string>
SessionServiceImpl::Submit(const std::vector<Common::TaskPayload> &task_requests,
                           const std::shared_ptr<IServiceInvocationHandler> &handler,
                           const Common::TaskOptions &task_options) {
  std::vector<armonik::api::grpc::v1::TaskRequest> definitions;
  definitions.reserve(task_requests.size());

  // Creates the needed result ids
  auto result_ids = generate_result_ids(task_requests.size());

  for (int i = 0; i < task_requests.size(); i++) {
    armonik::api::grpc::v1::TaskRequest request;
    // Serialize the request in an ArmoniK format
    *request.mutable_payload() = task_requests[i].Serialize();
    // One result per task
    request.add_expected_output_keys(result_ids[i]);
    // Set the data dependencies
    request.mutable_data_dependencies()->Add(task_requests[i].data_dependencies.begin(),
                                             task_requests[i].data_dependencies.end());

    definitions.push_back(request);
  }
  // Submit the tasks
  auto list =
      client->create_tasks_async(session, static_cast<armonik::api::grpc::v1::TaskOptions>(task_options), definitions)
          .get()
          .creation_status_list()
          .creation_statuses();
  std::vector<std::string> task_ids;
  task_ids.reserve(task_requests.size());

  {
    std::lock_guard lock(maps_mutex);
    for (auto &&t : list) {
      task_ids.push_back(t.task_info().task_id());
      taskId_resultId[t.task_info().task_id()] = t.task_info().expected_output_keys(0);
      resultId_taskId[t.task_info().expected_output_keys(0)] = t.task_info().task_id();
      result_handlers[t.task_info().expected_output_keys(0)] = handler;
    }
  }

  // Get the list of task ids
  std::transform(list.begin(), list.end(), std::back_inserter(task_ids),
                 [](const armonik::api::grpc::v1::submitter::CreateTaskReply_CreationStatus &status) {
                   return status.task_info().task_id();
                 });
  return task_ids;
}

SessionServiceImpl::SessionServiceImpl(const Common::Properties &properties)
    : taskOptions(properties.taskOptions), channel_pool(properties) {
  client = std::move(std::make_unique<Api::Client::SubmitterClient>(
      armonik::api::grpc::v1::submitter::Submitter::NewStub(channel_pool.GetChannel())));
  results = std::move(armonik::api::grpc::v1::results::Results::NewStub(channel_pool.GetChannel()));

  // Creates a new session
  session = client->create_session(static_cast<armonik::api::grpc::v1::TaskOptions>(properties.taskOptions),
                                   {properties.taskOptions.partition_id});
}

std::vector<std::string> SessionServiceImpl::Submit(const std::vector<Common::TaskPayload> &task_requests,
                                                    const std::shared_ptr<IServiceInvocationHandler> &handler) {
  return std::move(Submit(task_requests, handler, taskOptions));
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
      std::shared_lock _(maps_mutex);
      resultIds.reserve(resultId_taskId.size());
      for (auto &&[rid, tid] : resultId_taskId) {
        resultIds.push_back(rid);
      }
    }

    // If there is no task to wait on, return
    if (resultIds.empty()) {
      break;
    }

    bool hasError = false;
    // Ask for the results statuses
    auto statuses = client->get_result_status(session, resultIds);

    std::vector<std::pair<std::string, std::string>> done;

    // For each status
    for (auto &&[rid, status] : statuses) {
      std::shared_ptr<IServiceInvocationHandler> handler;
      std::string task_id;
      if (status == armonik::api::grpc::v1::result_status::RESULT_STATUS_COMPLETED ||
          status == armonik::api::grpc::v1::result_status::RESULT_STATUS_ABORTED) {
        // Get the handler and taskid information
        {
          std::shared_lock _(maps_mutex);
          handler = result_handlers.at(rid);
          task_id = resultId_taskId.at(rid);
        }

        armonik::api::grpc::v1::ResultRequest resultRequest;
        resultRequest.set_session(session);
        resultRequest.set_result_id(rid);

        try {
          // Get the finished result
          auto payload = client->get_result_async(resultRequest).get();
          if (status == armonik::api::grpc::v1::result_status::RESULT_STATUS_ABORTED) {
            // The above command should have failed !
            throw ArmoniK::Api::Common::exceptions::ArmoniKApiException(
                "Result is aborted and shouldn't have been retrieved");
          }

          // Handle the result
          handler->HandleResponse(payload, task_id);
        } catch (const ArmoniK::Api::Common::exceptions::ArmoniKTaskError &e) {
          // Task is in error
          hasError |= task_ids.find(task_id) != task_ids.end();
          std::cerr << "Task " << task_id << " is in error : " << e.what() << std::endl;
          handler->HandleError(e, task_id);

        } catch (const ArmoniK::Api::Common::exceptions::ArmoniKApiException &e) {
          // Internal error
          hasError |= task_ids.find(task_id) != task_ids.end();
          std::cerr << "Internal error while handling result " << e.what() << std::endl;
          handler->HandleError(e, task_id);
        }
        done.emplace_back(task_id, rid);
      }
    }

    {
      // Remove all finished tasks and results from the global lists
      std::lock_guard _(maps_mutex);
      for (auto &&[tid, rid] : done) {
        taskId_resultId.erase(tid);
        resultId_taskId.erase(rid);
        result_handlers.erase(rid);
      }
    }

    // Remove all finished task_ids from the waiting list
    for (auto &&[tid, rid] : done) {
      task_ids.erase(tid);
    }

    // If we wait for any and at least one is done, or if we break on error and had an error, then return
    if ((stopOnFirst && task_ids.size() < initialTaskIds_size) || (breakOnError && hasError)) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(options.polling_ms));
  }
}

} // namespace SDK_CLIENT_NAMESPACE::Internal
