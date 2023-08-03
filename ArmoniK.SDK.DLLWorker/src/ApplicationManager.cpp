#include "ApplicationManager.h"
#include "ArmoniKSdkException.h"
#include "Configuration.h"
#include <sstream>
SDK_DLLWORKER_NAMESPACE::ApplicationManager &
SDK_DLLWORKER_NAMESPACE::ApplicationManager::UseApplication(const AppId &appId) & {
  if (appId == currentId) {
    return *this;
  }
  service_manager.clear();
  std::string filename(applicationsBasePath + '/' + appId.application_name +
                       (appId.application_version.empty() ? "" : "." + appId.application_version));
  currentLibrary = DynamicLib(filename.c_str());

  functionPointers = ArmoniKFunctionPointers{currentLibrary.get<armonik_create_service_t>("armonik_create_service"),
                                             currentLibrary.get<armonik_destroy_service_t>("armonik_destroy_service"),
                                             currentLibrary.get<armonik_enter_session_t>("armonik_enter_session"),
                                             currentLibrary.get<armonik_leave_session_t>("armonik_leave_session"),
                                             currentLibrary.get<armonik_call_t>("armonik_call")};
  currentId = appId;
  std::cout << "Successfully loaded application " << appId.application_name << " ( " << appId.application_version
            << " )" << std::endl;
  return *this;
}
SDK_DLLWORKER_NAMESPACE::ApplicationManager &
SDK_DLLWORKER_NAMESPACE::ApplicationManager::UseService(const ServiceId &serviceId) & {
  if (!service_manager.matches(serviceId)) {
    service_manager = ServiceManager(functionPointers, serviceId);
  }

  return *this;
}
SDK_DLLWORKER_NAMESPACE::ApplicationManager &
SDK_DLLWORKER_NAMESPACE::ApplicationManager::UseSession(const std::string &sessionId) & {
  service_manager.UseSession(sessionId);
  return *this;
}
ArmoniK::Api::Worker::ProcessStatus
SDK_DLLWORKER_NAMESPACE::ApplicationManager::Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler,
                                                     const std::string &method_name,
                                                     const std::string &method_arguments) & {
  return service_manager.Execute(taskHandler, method_name, method_arguments);
}
SDK_DLLWORKER_NAMESPACE::ApplicationManager::ApplicationManager(const ArmoniK::Sdk::Common::Configuration &config)
    : functionPointers() {
  applicationsBasePath = config.get("Worker__ApplicationBasePath");
  if (applicationsBasePath.empty()) {
    applicationsBasePath = "/data";
  }
}
