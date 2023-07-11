#ifndef ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
#define ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H

#include "TaskOptions.h"
#include "TaskRequest.h"
#include <armonik/client/results_service.grpc.pb.h>
#include <armonik/client/submitter/SubmitterClient.h>
#include <armonik/common/objects.pb.h>
#include <grpc++/grpc++.h>
namespace SDK_CLIENT_NAMESPACE {

class SessionService {
private:
  std::string session;
  SubmitterClient client;
  std::unique_ptr<armonik::api::grpc::v1::results::Results::Stub> results;
  armonik::api::grpc::v1::TaskOptions taskOptions;

  std::vector<std::string> generate_result_ids(size_t num);

public:
  [[maybe_unused]] explicit SessionService(
      const std::shared_ptr<grpc::Channel> &channel,
      const armonik::api::grpc::v1::TaskOptions &taskOptions = get_default_task_option());

  SessionService() = delete;

  [[maybe_unused]] std::vector<std::string> Submit(const std::vector<TaskRequest> &requests);

  std::string getSession() const;
};
} // namespace SDK_CLIENT_NAMESPACE
#endif // ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
