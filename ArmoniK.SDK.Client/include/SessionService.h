#pragma once

#include "WaitBehavior.h"
#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
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
  std::unique_ptr<SDK_CLIENT_NAMESPACE::Internal::SessionServiceImpl> impl;
  ArmoniK::Api::Common::logger::LocalLogger logger_;
  void ensure_valid() const;

public:
  /**
   * @brief Creates a SessionService from the given Properties
   * @param properties Session properties
   */
  explicit SessionService(const ArmoniK::Sdk::Common::Properties &properties,
                          ArmoniK::Api::Common::logger::Logger &logger);
  SessionService(const SessionService &) = delete;
  /**
   * @brief Move constructor
   * @param other Other session service
   */
  SessionService(SessionService &&other) noexcept;
  SessionService &operator=(const SessionService &) = delete;
  /**
   * @brief Move assignment operator
   * @param other Other session service
   * @return this
   */
  SessionService &operator=(SessionService &&other) noexcept;
  ~SessionService();

  /**
   * @brief Submits the given list of task requests
   * @param requests List of task requests
   * @param handler Result handler for this batch of requests
   * @param task_options Task options to use for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskPayload> &requests,
                                  std::shared_ptr<IServiceInvocationHandler> handler,
                                  const ArmoniK::Sdk::Common::TaskOptions &task_options);

  /**
   * @brief Submits the given list of task requests using the session's task options
   * @param requests List of task requests
   * @param handler Result handler for this batch of requests
   * @return List of task ids
   */
  std::vector<std::string> Submit(const std::vector<Common::TaskPayload> &requests,
                                  std::shared_ptr<IServiceInvocationHandler> handler);

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
  [[nodiscard]] const std::string &getSession() const;
};
} // namespace SDK_CLIENT_NAMESPACE
