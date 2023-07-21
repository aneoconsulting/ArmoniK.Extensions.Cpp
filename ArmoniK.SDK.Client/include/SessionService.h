#ifndef ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
#define ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H

#include "WaitBehavior.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ArmoniK::SDK::Common {
class TaskOptions;
class Properties;
class TaskRequest;
} // namespace ArmoniK::SDK::Common

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
  explicit SessionService(const ArmoniK::SDK::Common::Properties &properties);

  /**
   * @brief Submits the given list of task requests
   * @param requests List of task requests
   * @param handler Result handler for this batch of requests
   * @param task_options Task options to use for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskRequest> &requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler,
                                  const ArmoniK::SDK::Common::TaskOptions &task_options);

  /**
   * @brief Submits the given list of task requests using the session's task options
   * @param requests List of task requests
   * @param handler Result handler for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskRequest> &requests,
                                  const std::shared_ptr<IServiceInvocationHandler> &handler);

  /**
   * @brief Waits for the completion of the given tasks
   * @param task_ids Task ids to wait on
   * @param options Wait options
   */
  void WaitResults(std::vector<std::string> task_ids, WaitOptions options = WaitOptions());

  /**
   * @brief Waits for the completion of all remaining tasks
   */
  void WaitResults();

  /**
   * @brief Get the session Id associated with this service
   * @return Session Id
   */
  [[nodiscard]] std::string_view getSession() const;
};
} // namespace SDK_CLIENT_NAMESPACE
#endif // ARMONIK_EXTENSIONS_CPP_SESSIONSERVICE_H
