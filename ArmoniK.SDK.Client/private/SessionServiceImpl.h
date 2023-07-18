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
/**
 * @brief Private implementation of the Session Service
 */
class SessionServiceImpl {
private:
  /**
   * @brief Requests the control plane to create results
   * @param num Number of results to create
   * @return List of result ids
   */
  std::vector<std::string> generate_result_ids(size_t num);
  /**
   * @brief Session
   */
  std::string session;
  /**
   * @brief Session task options
   */
  Common::TaskOptions taskOptions;

  /**
   * @brief Client used for submission
   */
  std::unique_ptr<ArmoniK::Api::Client::SubmitterClient> client;

  /**
   * @brief Client used for results handling
   */
  std::unique_ptr<armonik::api::grpc::v1::results::Results::Stub> results;

  /**
   * @brief Channel pool
   */
  ChannelPool channel_pool;

public:
  SessionServiceImpl() = delete;
  SessionServiceImpl(const SessionServiceImpl &) = delete;

  /**
   * @brief Creates a SessionService from the given Properties
   * @param properties Session properties
   */
  explicit SessionServiceImpl(const ArmoniK::SDK::Common::Properties &properties);

  /**
   * @brief Submits the given list of task requests using the session's task options
   * @param task_requests List of task requests
   * @param handler Result handler for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskRequest> &task_requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler);

  /**
   * @brief Submits the given list of task requests
   * @param task_requests List of task requests
   * @param handler Result handler for this batch of requests
   * @param task_options Task options to use for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskRequest> &task_requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler,
                                  const Common::TaskOptions &task_options);

  /**
   * @brief Get the session Id associated with this service
   * @return Session Id
   */
  [[nodiscard]] std::string_view getSession() const;
};
} // namespace SDK_CLIENT_NAMESPACE::Internal

#endif // ARMONIK_SDK_SESSIONSERVICEIMPL_H
