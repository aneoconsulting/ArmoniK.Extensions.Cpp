#pragma once

#include "WaitBehavior.h"
#include <armonik/common/logger/formatter.h>
#include <armonik/common/logger/logger.h>
#include <armonik/common/logger/writer.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ArmoniK {
namespace Sdk {
namespace Common {
struct TaskOptions;
struct Properties;
struct TaskPayload;
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK

namespace ArmoniK {
namespace Sdk {
namespace Client {
class IServiceInvocationHandler;
namespace Internal {
class SessionServiceImpl;
}

/**
 * @brief Service used for task submission
 */
class SessionService {
private:
  std::unique_ptr<ArmoniK::Sdk::Client::Internal::SessionServiceImpl> impl;
  armonik::api::common::logger::LocalLogger logger_;
  void ensure_valid() const;

public:
  /**
   * @brief Creates a SessionService from the given Properties
   * @param properties Session properties
   * @param logger logger
   * @param session_id session id to open, leave blank to open a new session
   */
  explicit SessionService(const ArmoniK::Sdk::Common::Properties &properties,
                          armonik::api::common::logger::Logger &logger, const std::string &session_id = "");
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

  /**
   * @brief Discards all results, cancels the tasks and cancels the session. No handler will be called.
   * @warning The session and its data will not be recoverable.
   * @warning Using this on a session with running or pending tasks will result in undefined behavior
   */
  void DropSession();

  /**
   * @brief Discards the results data of the given tasks. The associated results must be completed or aborted.
   * @param task_ids Task ids
   * @warning The data of these results will not be recoverable. Tasks which depend on these data will fail.
   * @warning The tasks will not be processed by the client.
   */
  void CleanupTasks(std::vector<std::string> task_ids);
};
} // namespace Client
} // namespace Sdk
} // namespace ArmoniK
