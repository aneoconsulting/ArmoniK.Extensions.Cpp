#pragma once

#include "ChannelPool.h"
#include "armonik/sdk/client/WaitBehavior.h"
#include <armonik/client/submitter/SubmitterClient.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <mutex>
#include <results_service.grpc.pb.h>

namespace ArmoniK {
namespace Sdk {
namespace Common {
struct Properties;
struct TaskPayload;
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK

namespace ArmoniK {
namespace Sdk {
namespace Client {
class IServiceInvocationHandler;
}
} // namespace Sdk
} // namespace ArmoniK

namespace ArmoniK {
namespace Sdk {
namespace Client {
namespace Internal {
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
  ArmoniK::Sdk::Common::TaskOptions taskOptions;

  /**
   * @brief Map between taskId and resultId
   */
  std::map<std::string, std::string> taskId_resultId;

  /**
   * @brief Map between result id and task id
   */
  std::map<std::string, std::string> resultId_taskId;

  /**
   * @brief Map between a result_id and it's handler
   */
  std::map<std::string, std::shared_ptr<IServiceInvocationHandler>> result_handlers;

  /**
   * @brief Maps mutex
   */
  std::mutex maps_mutex;

  /**
   * @brief Channel pool
   */
  ChannelPool channel_pool;

  /**
   * @brief Local logger
   *
   */
  armonik::api::common::logger::LocalLogger logger_;

public:
  SessionServiceImpl() = delete;
  SessionServiceImpl(const SessionServiceImpl &) = delete;

  /**
   * @brief Creates a SessionService from the given Properties
   * @param properties Session properties
   */
  explicit SessionServiceImpl(const ArmoniK::Sdk::Common::Properties &properties,
                              armonik::api::common::logger::Logger &logger, const std::string &session_id = "");

  /**
   * @brief Submits the given list of task requests using the session's task options
   * @param task_requests List of task requests
   * @param handler Result handler for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskPayload> &task_requests,
                                  std::shared_ptr<IServiceInvocationHandler> handler);

  /**
   * @brief Submits the given list of task requests
   * @param task_requests List of task requests
   * @param handler Result handler for this batch of requests
   * @param task_options Task options to use for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskPayload> &task_requests,
                                  std::shared_ptr<IServiceInvocationHandler> handler,
                                  const Common::TaskOptions &task_options);

  /**
   * @brief Get the session Id associated with this service
   * @return Session Id
   */
  [[nodiscard]] const std::string &getSession() const;

  /**
   * @brief Waits for the completion of the given tasks
   * @param task_ids Task ids to wait on. If left empty, will wait for submitted tasks in
   * @param waitBehavior Wait for all tasks completion, any task completion and/or stop waiting if a result is aborted
   * @param options Wait options
   * @note If tasks are being submitted concurrently with the wait, this function may return before the concurrent tasks
   * submission is complete
   */
  void WaitResults(std::set<std::string> task_ids = {}, WaitBehavior waitBehavior = All,
                   const WaitOptions &options = WaitOptions());

  /**
   * @brief Discards all results (payload, intermediate and final) of the session. Used for cleanup.
   * @note All the results will be deleted from the session and will not be recoverable. This may cause running or
   * pending tasks to fail in an unrecoverable manner.
   */
  void DropSession();

  /**
   * @brief Discards the results data of the given tasks. The associated results must be completed or aborted.
   * @param task_ids Task ids
   * @warning The data of these results will not be recoverable. Tasks which depend on these data will fail.
   * @warning If the given task has not been processed, the behavior is undefined. The tasks will not be processed by
   * the client.
   */
  void CleanupTasks(const std::vector<std::string> &task_ids);
};
} // namespace Internal
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
