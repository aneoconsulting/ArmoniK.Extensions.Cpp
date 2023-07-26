#pragma once
#include "ContextIds.h"
#include "ServiceManager.h"
#include <armonik/worker/Worker/TaskHandler.h>

namespace SDK_WORKER_NAMESPACE {
class ApplicationManager {

public:
  ApplicationManager() : functionPointers(){};
  ApplicationManager(const ApplicationManager &) = delete;
  void operator=(ApplicationManager const &) = delete;

  ApplicationManager &UseApplication(const AppId &appId);
  ApplicationManager &UseService(const ServiceId &serviceId);
  ApplicationManager &UseSession(const std::string &sessionId);
  armonik::api::grpc::v1::Output Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler, const std::string &method_name,
                                         const std::string &method_arguments);

private:
  ArmoniKFunctionPointers functionPointers;
  AppId currentId;
  std::unique_ptr<ServiceManager> service_manager;
  // TODO Add dynamic DLL requisites
};
} // namespace SDK_WORKER_NAMESPACE
