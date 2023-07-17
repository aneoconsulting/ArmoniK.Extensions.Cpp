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

  std::vector<armonik::api::grpc::v1::results::CreateResultsMetaDataRequest_ResultCreate> results_create;
  results_create.reserve(num);
  for (int i = 0; i < num; i++) {
    armonik::api::grpc::v1::results::CreateResultsMetaDataRequest_ResultCreate result_create;
    *result_create.mutable_name() = armonik::api::common::utils::GuuId::generate_uuid();
    results_create.push_back(result_create);
  }

  results_request.mutable_results()->Add(results_create.begin(), results_create.end());
  *results_request.mutable_session_id() = session;

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

  auto result_ids = generate_result_ids(task_requests.size());

  for (int i = 0; i < task_requests.size(); i++) {
    armonik::api::grpc::v1::TaskRequest request;
    *request.mutable_payload() = task_requests[i].Serialize();
    request.add_expected_output_keys(result_ids[i]);
    request.mutable_data_dependencies()->Add(task_requests[i].data_dependencies.begin(),
                                             task_requests[i].data_dependencies.end());
  }
  auto list =
      client->create_tasks_async(session, static_cast<armonik::api::grpc::v1::TaskOptions>(task_options), definitions)
          .get()
          .creation_status_list()
          .creation_statuses();
  std::vector<std::string> task_ids;
  task_ids.reserve(task_requests.size());

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

  session = client->create_session(static_cast<armonik::api::grpc::v1::TaskOptions>(properties.taskOptions),
                                   {properties.taskOptions.partition_id});
}

std::vector<std::string> SessionServiceImpl::Submit(const std::vector<Common::TaskRequest> &task_requests,
                                                    const std::shared_ptr<IServiceInvocationHandler> &handler) {
  return std::move(Submit(task_requests, handler, taskOptions));
}
} // namespace SDK_CLIENT_NAMESPACE::Internal
