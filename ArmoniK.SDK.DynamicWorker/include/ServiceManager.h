#pragma once

#include "ContextIds.h"
#include <armonik/worker/Worker/ProcessStatus.h>
#include <armonik/worker/Worker/TaskHandler.h>

namespace SDK_DYNAMICWORKER_NAMESPACE {
class ServiceManager {
public:
  ServiceManager() = default;
  ServiceManager(ArmoniKFunctionPointers functionsPointers, ServiceId serviceId);
  ~ServiceManager();

  ServiceManager(const ServiceManager &) = delete;
  ServiceManager &operator=(const ServiceManager &) = delete;

  ServiceManager(ServiceManager &&other) noexcept
      : serviceId(std::move(other.serviceId)), current_session(std::move(other.current_session)),
        service_context(other.service_context), session_context(other.session_context),
        functionPointers(other.functionPointers) {
    other.service_context = nullptr;
    other.session_context = nullptr;
    other.serviceId.clear();
  }
  ServiceManager &operator=(ServiceManager &&other) noexcept {
    using std::swap;
    swap(serviceId, other.serviceId);
    swap(current_session, other.current_session);
    swap(service_context, other.service_context);
    swap(session_context, other.session_context);
    swap(functionPointers, other.functionPointers);
    return *this;
  }

  ServiceManager &UseSession(const std::string &sessionId) &;
  ArmoniK::Api::Worker::ProcessStatus Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler,
                                              const std::string &method_name, const std::string &method_arguments);

  bool matches(const ServiceId &service_id);
  void clear();

private:
  ServiceId serviceId;
  std::string current_session;
  void *service_context{};
  void *session_context{};
  ArmoniKFunctionPointers functionPointers{};

  static void UploadResult(void *opaque_task_handler, armonik_status_t status, const char *data, size_t data_size);
};
} // namespace SDK_DYNAMICWORKER_NAMESPACE
