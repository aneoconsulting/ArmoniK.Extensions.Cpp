#include "SessionServiceImpl.h"
#include "IServiceInvocationHandler.h"
#include "Properties.h"
#include "TaskRequest.h"
#include <armonik/client/results_common.pb.h>
#include <armonik/common/utils/GuuId.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <objects.pb.h>
#include <string>
#include <submitter_common.pb.h>
#include <submitter_service.grpc.pb.h>
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
    *result_create.mutable_name() = armonik::api::common::utils::GuuId::generate_uuid();
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
    throw std::runtime_error(message.str().c_str());
  }
  std::vector<std::string> result_ids;
  result_ids.reserve(num);
  // Get the result ids from the response
  std::transform(results_response.results().begin(), results_response.results().end(), std::back_inserter(result_ids),
                 [](auto res) { return res.result_id(); });
  return result_ids;
}

std::string_view SessionServiceImpl::getSession() const { return session; }

[[maybe_unused]] std::vector<std::string>
SessionServiceImpl::Submit(const std::vector<Common::TaskRequest> &task_requests,
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
    for(auto && t : list){
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
  client_stub = armonik::api::grpc::v1::submitter::Submitter::NewStub(channel_pool.GetChannel());
  client = std::move(std::make_unique<Api::Client::SubmitterClient>(armonik::api::grpc::v1::submitter::Submitter::NewStub(channel_pool.GetChannel())));
  results = std::move(armonik::api::grpc::v1::results::Results::NewStub(channel_pool.GetChannel()));

  // Creates a new session
  session = client->create_session(static_cast<armonik::api::grpc::v1::TaskOptions>(properties.taskOptions),
                                   {properties.taskOptions.partition_id});
}

std::vector<std::string> SessionServiceImpl::Submit(const std::vector<Common::TaskRequest> &task_requests,
                                                    const std::shared_ptr<IServiceInvocationHandler> &handler) {
  return std::move(Submit(task_requests, handler, taskOptions));
}

void SessionServiceImpl::WaitResults(const std::vector<std::string>& task_ids, WaitBehavior waitBehavior, const WaitOptions& options) {
  std::vector<std::string> results_to_wait;
  results_to_wait.reserve(task_ids.size());
  {
    std::lock_guard lock(maps_mutex);
    for(auto&& t : task_ids){
      auto kp = taskId_resultId.find(t);
      if(kp != taskId_resultId.end()){
        results_to_wait.push_back(kp->second);
      }
    }
  }
  WaitListOfResults(results_to_wait, waitBehavior, options);
}

void SessionServiceImpl::WaitResults(const WaitOptions& options) {
  std::vector<std::string> result_ids;
  while(true){
    {
      std::lock_guard lock(maps_mutex);
      if(taskId_resultId.empty()){
        break;
      }
      result_ids.reserve(taskId_resultId.size());
      for (auto &&[_, v] : taskId_resultId) {
        result_ids.push_back(v);
      }
    }
    WaitListOfResults(result_ids, WaitBehavior::All, options);
    result_ids.clear();
  }

}
void SessionServiceImpl::WaitListOfResults(const std::vector<std::pair<std::string, std::string>>& ids, WaitBehavior behavior,
                                           WaitOptions options) {
  if(ids.empty()){
    return;
  }

  bool breakOnError = behavior & WaitBehavior::BreakOnError;
  bool stopOnFirst = behavior & WaitBehavior::Any;

  grpc::ClientContext context;
  armonik::api::grpc::v1::submitter::GetResultStatusRequest request;
  armonik::api::grpc::v1::submitter::GetResultStatusReply reply;
  request.set_session_id(session);
  request.mutable_result_ids()->Reserve(ids.size());

  for(auto&& [tid, rid] : ids){
    *request.mutable_result_ids()->Add() = rid;
  }

  while(!request.result_ids().empty()){
    auto status = client_stub->GetResultStatus(&context, request, &reply);
    if(!status.ok()){
      throw std::runtime_error("Error while getting result status : " + status.error_message());
    }

    for(auto&& result_status : reply.id_statuses()){
      if(result_status.status() == armonik::api::grpc::v1::result_status::RESULT_STATUS_COMPLETED){
        try{
          armonik::api::grpc::v1::ResultRequest resultRequest;
          resultRequest.set_session(session);
          resultRequest.set_result_id(result_status.result_id());
          auto raw_payload = client->get_result_async(resultRequest).get();
          std::string_view payload(reinterpret_cast<char*>(raw_payload.data()), raw_payload.size());

          {
            std::lock_guard _(maps_mutex);
            result_handlers.at(result_status.result_id())->HandleResponse(payload, resultId_taskId.at(result_status.result_id()));
          }
        }catch(const std::exception& e){

        }
      }
    }
  }




}
} // namespace SDK_CLIENT_NAMESPACE::Internal
