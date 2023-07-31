#pragma once

#include "ContextIds.h"
#include <armonik/worker/Worker/ProcessStatus.h>
#include <armonik/worker/Worker/TaskHandler.h>

namespace SDK_WORKER_NAMESPACE {
class ServiceManager {
public:
  ServiceManager(ArmoniKFunctionPointers functionsPointers, ServiceId serviceId);
  ~ServiceManager();

  ServiceManager &UseSession(const std::string &sessionId);
  ArmoniK::Api::Worker::ProcessStatus Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler,
                                              const std::string &method_name, const std::string &method_arguments);

private:
  ServiceId serviceId;
  std::string current_session;
  void *service_context{};
  void *session_context{};
  ArmoniKFunctionPointers functionPointers;

  static void UploadResult(void *opaque_task_handler, armonik_status_t status, const char *data, size_t data_size);
};
} // namespace SDK_WORKER_NAMESPACE
