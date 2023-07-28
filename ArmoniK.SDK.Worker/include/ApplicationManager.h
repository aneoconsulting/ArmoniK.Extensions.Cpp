#pragma once
#include "ContextIds.h"
#include "ServiceManager.h"
#include <armonik/worker/Worker/TaskHandler.h>

namespace ArmoniK::Sdk::Common {
class IConfiguration;
}

namespace SDK_WORKER_NAMESPACE {
class ApplicationManager {

public:
  explicit ApplicationManager(const ArmoniK::Sdk::Common::IConfiguration &config);
  ApplicationManager(const ApplicationManager &) = delete;
  void operator=(ApplicationManager const &) = delete;

  ApplicationManager &UseApplication(const AppId &appId);
  ApplicationManager &UseService(const ServiceId &serviceId);
  ApplicationManager &UseSession(const std::string &sessionId);
  ArmoniK::Api::Worker::ProcessStatus Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler,
                                              const std::string &method_name, const std::string &method_arguments);

private:
  ArmoniKFunctionPointers functionPointers;
  AppId currentId;
  std::unique_ptr<ServiceManager> service_manager;
  void *applicationHandle = nullptr;
  std::string applicationsBasePath;
};
} // namespace SDK_WORKER_NAMESPACE
