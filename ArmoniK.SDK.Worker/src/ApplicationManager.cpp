#include "ApplicationManager.h"
#include "ArmoniKSDKException.h"
#include "IConfiguration.h"
#include <dlfcn.h>
#include <sstream>
SDK_WORKER_NAMESPACE::ApplicationManager &SDK_WORKER_NAMESPACE::ApplicationManager::UseApplication(const AppId &appId) {
  if (appId == currentId) {
    return *this;
  }
  if (service_manager) {
    service_manager.reset();
  }
  if (!currentId.empty() && applicationHandle) {
    currentId.application_version.clear();
    currentId.application_name.clear();
    functionPointers.clear();
    dlclose(applicationHandle);
  }
  std::string filename(applicationsBasePath + '/' + appId.application_name +
                       (appId.application_version.empty() ? "" : "." + appId.application_version));
  auto handle = dlopen(filename.c_str(), RTLD_LAZY);
  if (handle == nullptr) {
    throw ArmoniK::Sdk::Common::ArmoniKSDKException("Could not load application " + filename);
  }

  functionPointers = ArmoniKFunctionPointers((armonik_create_service_t)dlsym(handle, "armonik_create_service"),
                                             (armonik_destroy_service_t)dlsym(handle, "armonik_destroy_service"),
                                             (armonik_enter_session_t)dlsym(handle, "armonik_enter_session"),
                                             (armonik_leave_session_t)dlsym(handle, "armonik_leave_session"),
                                             (armonik_call_t)dlsym(handle, "armonik_call"));
  if (!(functionPointers.enter_session && functionPointers.leave_session && functionPointers.create_service &&
        functionPointers.destroy_service && functionPointers.call)) {
    std::stringstream ss;
    ss << "Loaded application " << filename << " doesn't implement all the required functions : ";
    ss << "\n armonik_create_service : " << (functionPointers.create_service ? "Found" : "Not Found");
    ss << "\n armonik_destroy_service : " << (functionPointers.destroy_service ? "Found" : "Not Found");
    ss << "\n armonik_enter_session : " << (functionPointers.enter_session ? "Found" : "Not Found");
    ss << "\n armonik_leave_session : " << (functionPointers.leave_session ? "Found" : "Not Found");
    ss << "\n armonik_call : " << (functionPointers.call ? "Found" : "Not Found");
    dlclose(handle);
    throw ArmoniK::Sdk::Common::ArmoniKSDKException(ss.str());
  }
  currentId = appId;
  applicationHandle = handle;
  std::cout << "Successfully loaded application " << appId.application_name << " ( " << appId.application_version
            << " )" << std::endl;
  return *this;
}
SDK_WORKER_NAMESPACE::ApplicationManager &
SDK_WORKER_NAMESPACE::ApplicationManager::UseService(const ServiceId &serviceId) {
  if (!service_manager || !service_manager->matches(serviceId)) {
    service_manager = std::make_unique<ServiceManager>(functionPointers, serviceId);
  }

  return *this;
}
SDK_WORKER_NAMESPACE::ApplicationManager &
SDK_WORKER_NAMESPACE::ApplicationManager::UseSession(const std::string &sessionId) {
  if (!service_manager) {
    throw ArmoniK::Sdk::Common::ArmoniKSDKException("Service is not initialized");
  }
  service_manager->UseSession(sessionId);
  return *this;
}
ArmoniK::Api::Worker::ProcessStatus
SDK_WORKER_NAMESPACE::ApplicationManager::Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler,
                                                  const std::string &method_name, const std::string &method_arguments) {
  if (!service_manager) {
    throw ArmoniK::Sdk::Common::ArmoniKSDKException("Service is not initialized");
  }
  return service_manager->Execute(taskHandler, method_name, method_arguments);
}
SDK_WORKER_NAMESPACE::ApplicationManager::ApplicationManager(const ArmoniK::Sdk::Common::IConfiguration &config)
    : functionPointers() {
  applicationsBasePath = config.get("Worker__ApplicationBasePath");
  if (applicationsBasePath.empty()) {
    applicationsBasePath = "/data";
  }
}
