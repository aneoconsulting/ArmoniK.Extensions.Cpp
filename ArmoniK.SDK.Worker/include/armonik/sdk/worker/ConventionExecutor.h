#pragma once

#include "ServiceBase.h"
#include <armonik/common/utils/string_view.h>
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/ConventionTask.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/worker/Worker/ProcessStatus.h>
#include <armonik/worker/Worker/TaskHandler.h>
#include <string>

namespace ArmoniK {
namespace Sdk {
namespace Worker {

/**
 * @brief Runs convention tasks on a service compiled into the worker (no dynamic library loading).
 *
 * Use it from the Execute method of a worker derived from armonik::api::worker::ArmoniKWorker:
 * @code
 * armonik::api::worker::ProcessStatus Execute(armonik::api::worker::TaskHandler &taskHandler) override {
 *   return executor.Execute(taskHandler);
 * }
 * @endcode
 * Tasks are accepted from any SDK client (C++, Java, C#). The library keys (LibraryPath, LibraryBlobId)
 * are ignored, so a C++ client only needs TaskOptions::SetConvention.
 *
 * @note This header is not compiled into ArmoniK.SDK.Worker, so the library keeps no dependency on
 *       ArmoniK.Api.Worker. Including it requires ArmoniK.Api.Worker and ArmoniK.SDK.Common.
 */
class ConventionExecutor {
public:
  /**
   * @brief Creates an executor for the given service
   * @param service Service to call. Must outlive the executor.
   */
  explicit ConventionExecutor(ServiceBase &service) : service(service) {}
  ~ConventionExecutor() {
    if (!current_session.empty()) {
      service.leave_session(session_context);
    }
  }

  ConventionExecutor(const ConventionExecutor &) = delete;
  ConventionExecutor &operator=(const ConventionExecutor &) = delete;

  /**
   * @brief Executes a convention task
   * @param taskHandler ArmoniK task handler
   * @return Task execution status. An ArmoniKSdkException gives a failed status; other exceptions propagate,
   *         which makes ArmoniK retry the task, as with the DynamicWorker.
   */
  armonik::api::worker::ProcessStatus Execute(armonik::api::worker::TaskHandler &taskHandler) {
    try {
      const Common::TaskOptions opts(taskHandler.getTaskOptions());
      if (!Common::ConventionTask::IsConventionTask(opts)) {
        throw Common::ArmoniKSdkException(std::string("Not a convention task: missing '") +
                                          Common::DynamicLibrary::KeyConventionVersion + "' task option");
      }
      auto task = Common::ConventionTask::Decode(opts, taskHandler.getPayload(), taskHandler.getDataDependencies(),
                                                 taskHandler.getExpectedResults());
      if (task.outputs.size() != 1) {
        throw Common::ArmoniKSdkException("This worker runs only convention tasks with a single output");
      }

      UseSession(taskHandler.getSessionId());
      const auto output = service.call(session_context, task.method_name, task.inputs);
      taskHandler.send_result(task.outputs.begin()->second, armonik::api::string_view(output.data(), output.size()))
          .get();
      return {};
    } catch (const Common::ArmoniKSdkException &e) {
      return armonik::api::worker::ProcessStatus(e.what());
    }
  }

private:
  void UseSession(const std::string &session_id) {
    if (session_id != current_session) {
      if (!current_session.empty()) {
        service.leave_session(session_context);
        current_session.clear();
      }
      session_context = service.enter_session(session_id.c_str());
      current_session = session_id;
    }
  }

  ServiceBase &service;
  std::string current_session;
  void *session_context{};
};

} // namespace Worker
} // namespace Sdk
} // namespace ArmoniK
