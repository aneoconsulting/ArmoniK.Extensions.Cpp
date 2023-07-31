#ifndef ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
#define ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H

#include "WaitBehavior.h"
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ArmoniK::Sdk::Common {
class TaskOptions;
class Properties;
class TaskPayload;
} // namespace ArmoniK::Sdk::Common

namespace SDK_CLIENT_NAMESPACE::Internal {
class SessionServiceImpl;
}

namespace SDK_CLIENT_NAMESPACE {
class IServiceInvocationHandler;

/**
 * @brief Service used for task submission
 */
class SessionService {
private:
  std::shared_ptr<SDK_CLIENT_NAMESPACE::Internal::SessionServiceImpl> impl;

public:
  SessionService() = delete;
  /**
   * @brief Creates a SessionService from the given Properties
   * @param properties Session properties
   */
  explicit SessionService(const ArmoniK::Sdk::Common::Properties &properties);

  /**
   * @brief Submits the given list of task requests
   * @param requests List of task requests
   * @param handler Result handler for this batch of requests
   * @param task_options Task options to use for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskPayload> &requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler,
                                  const ArmoniK::Sdk::Common::TaskOptions &task_options);

  /**
   * @brief Submits the given list of task requests using the session's task options
   * @param requests List of task requests
   * @param handler Result handler for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskPayload> &requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler);

  /**
   * @brief Waits for the completion of the given tasks
   * @param task_ids Task ids to wait on. If left empty, will wait for submitted tasks in
   * @param waitBehavior Wait for all tasks completion, any task completion and/or stop waiting if a result is aborted
   * @param options Wait options
   * @note When waiting for all tasks to finish, if tasks are being submitted concurrently with the wait, this function
   * may return before the concurrent tasks submission is complete
   */
  void WaitResults(std::set<std::string> task_ids = {}, WaitBehavior waitBehavior = All,
                   const WaitOptions &options = WaitOptions());

  /**
   * @brief Get the session Id associated with this service
   * @return Session Id
   */
  [[nodiscard]] std::string_view getSession() const;
};
} // namespace SDK_CLIENT_NAMESPACE
#endif // ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
