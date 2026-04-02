#include "ApplicationManager.h"
#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/Configuration.h>
#include <armonik/sdk/common/TaskPayload.h>
#include <sstream>

namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {

ApplicationManager &ApplicationManager::UseApplication(const AppId &appId) & {
  if (appId == currentId) {
    return *this;
  }
  service_manager.clear();
  currentLibraryPath.clear();
  std::string filename(applicationsBasePath + '/' + appId.application_name +
                       (appId.application_version.empty() ? "" : "." + appId.application_version));
  currentLibrary = DynamicLib(filename.c_str());

  functionPointers = ArmoniKFunctionPointers{currentLibrary.get<armonik_create_service_t>("armonik_create_service"),
                                             currentLibrary.get<armonik_destroy_service_t>("armonik_destroy_service"),
                                             currentLibrary.get<armonik_enter_session_t>("armonik_enter_session"),
                                             currentLibrary.get<armonik_leave_session_t>("armonik_leave_session"),
                                             currentLibrary.get<armonik_call_t>("armonik_call")};
  currentId = appId;
  logger.info("Successfully loaded application " + appId.application_name + " ( " + appId.application_version + " )");
  return *this;
}
ApplicationManager &ApplicationManager::UseService(const ServiceId &serviceId) & {
  if (!service_manager.matches(serviceId)) {
    service_manager = ServiceManager(functionPointers, serviceId);
  }

  return *this;
}
ApplicationManager &ApplicationManager::UseSession(const std::string &sessionId) & {
  service_manager.UseSession(sessionId);
  return *this;
}
armonik::api::worker::ProcessStatus ApplicationManager::Execute(armonik::api::worker::TaskHandler &taskHandler,
                                                                const std::string &method_name,
                                                                const std::string &method_arguments) {
  return service_manager.Execute(taskHandler, method_name, method_arguments);
}

armonik::api::worker::ProcessStatus
ApplicationManager::Execute(armonik::api::worker::TaskHandler &taskHandler, const std::string &method_name,
                             const std::map<std::string, std::string> &inputs,
                             const std::map<std::string, std::string> &outputs) {
  ArmoniK::Sdk::Common::TaskPayload payload;
  payload.method_name = method_name;
  payload.inputs = inputs;
  payload.outputs = outputs;
  return Execute(taskHandler, method_name, payload.Serialize());
}

ApplicationManager &ApplicationManager::UseLibrary(const ArmoniK::Sdk::Common::DynamicLibrary &lib) & {
  if (lib.library_path == currentLibraryPath) {
    return *this;
  }
  service_manager.clear();
  currentId.clear();
  currentLibrary = DynamicLib(lib.library_path.c_str());

  const std::string prefix = lib.symbol.empty() ? "armonik" : lib.symbol;
  functionPointers =
      ArmoniKFunctionPointers{currentLibrary.get<armonik_create_service_t>((prefix + "_create_service").c_str()),
                              currentLibrary.get<armonik_destroy_service_t>((prefix + "_destroy_service").c_str()),
                              currentLibrary.get<armonik_enter_session_t>((prefix + "_enter_session").c_str()),
                              currentLibrary.get<armonik_leave_session_t>((prefix + "_leave_session").c_str()),
                              currentLibrary.get<armonik_call_t>((prefix + "_call").c_str())};

  currentLibraryPath = lib.library_path;
  // Create a default service (convention workers typically export a single unnamed service)
  service_manager = ServiceManager(functionPointers, ServiceId({lib.library_path, ""}, "", ""));
  logger.info("Successfully loaded library " + lib.library_path);
  return *this;
}
ApplicationManager::ApplicationManager(const ArmoniK::Sdk::Common::Configuration &config,
                                       const armonik::api::common::logger::Logger &logger)
    : functionPointers(), logger(logger.local()) {
  applicationsBasePath = config.get("Worker__ApplicationBasePath");
  if (applicationsBasePath.empty()) {
    applicationsBasePath = "/data";
  }
}
} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
