#ifndef ARMONIK_SDK_SESSIONSERVICEIMPL_H
#define ARMONIK_SDK_SESSIONSERVICEIMPL_H

#include "ChannelPool.h"
#include "TaskOptions.h"
#include <armonik/client/submitter/SubmitterClient.h>
#include <results_service.grpc.pb.h>

namespace ArmoniK::SDK::Common {
class Properties;
class TaskRequest;
} // namespace ArmoniK::SDK::Common

namespace SDK_CLIENT_NAMESPACE {
class IServiceInvocationHandler;
}

namespace SDK_CLIENT_NAMESPACE::Internal {
class SessionServiceImpl {
private:
  std::vector<std::string> generate_result_ids(size_t num);
  std::string session;
  Common::TaskOptions taskOptions;

  std::unique_ptr<ArmoniK::Api::Client::SubmitterClient> client;
  std::unique_ptr<armonik::api::grpc::v1::results::Results::Stub> results;
  ChannelPool channel_pool;

public:
  SessionServiceImpl() = delete;
  SessionServiceImpl(const SessionServiceImpl &) = delete;
  explicit SessionServiceImpl(const ArmoniK::SDK::Common::Properties &properties);

  std::vector<std::string> Submit(const std::vector<Common::TaskRequest> &task_requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler);
  std::vector<std::string> Submit(const std::vector<Common::TaskRequest> &task_requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler,
                                  const Common::TaskOptions &task_options);

  [[nodiscard]] std::string_view getSession() const;
};
} // namespace SDK_CLIENT_NAMESPACE::Internal

#endif // ARMONIK_SDK_SESSIONSERVICEIMPL_H
