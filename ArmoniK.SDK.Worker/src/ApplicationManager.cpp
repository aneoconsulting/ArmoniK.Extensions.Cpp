#include "ApplicationManager.h"
#include "ArmoniKSDKException.h"
SDK_WORKER_NAMESPACE::ApplicationManager &SDK_WORKER_NAMESPACE::ApplicationManager::UseApplication(const AppId &appId) {
  if (appId != currentId) {
    if (service_manager) {
      service_manager.reset();
    }
  }
  if (!currentId.empty()) {
    // TODO Destroy application
  }
  // TODO Load application and get function pointers
  currentId = appId;
  functionPointers = ArmoniKFunctionPointers(armonik_create_service, armonik_destroy_service, armonik_enter_session,
                                             armonik_leave_session, armonik_call);
  return *this;
}
SDK_WORKER_NAMESPACE::ApplicationManager &
SDK_WORKER_NAMESPACE::ApplicationManager::UseService(const ServiceId &serviceId) {
  service_manager = std::make_unique<ServiceManager>(functionPointers, serviceId);
  return *this;
}
SDK_WORKER_NAMESPACE::ApplicationManager &
SDK_WORKER_NAMESPACE::ApplicationManager::UseSession(const std::string &sessionId) {
  if (!service_manager) {
    throw ArmoniK::SDK::Common::ArmoniKSDKException("Service is not initialized");
  }
  service_manager->UseSession(sessionId);
  return *this;
}
ArmoniK::Api::Worker::ProcessStatus
SDK_WORKER_NAMESPACE::ApplicationManager::Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler,
                                                  const std::string &method_name, const std::string &method_arguments) {
  if (!service_manager) {
    throw ArmoniK::SDK::Common::ArmoniKSDKException("Service is not initialized");
  }
  return service_manager->Execute(taskHandler, method_name, method_arguments);
}
