#pragma once
#include "ContextIds.h"
#include "DynamicLib.h"
#include "ServiceManager.h"
#include <Worker/ProcessStatus.h>
#include <armonik/worker/Worker/TaskHandler.h>

namespace ArmoniK::Sdk::Common {
class Configuration;
}

namespace SDK_DYNAMICWORKER_NAMESPACE {
class ApplicationManager {

public:
  explicit ApplicationManager(const ArmoniK::Sdk::Common::Configuration &config);

  ApplicationManager &UseApplication(const AppId &appId) &;
  ApplicationManager &UseService(const ServiceId &serviceId) &;
  ApplicationManager &UseSession(const std::string &sessionId) &;
  ArmoniK::Api::Worker::ProcessStatus Execute(ArmoniK::Api::Worker::TaskHandler &taskHandler,
                                              const std::string &method_name, const std::string &method_arguments);

private:
  ArmoniKFunctionPointers functionPointers;
  AppId currentId;
  ServiceManager service_manager;
  DynamicLib currentLibrary;
  std::string applicationsBasePath;
};
} // namespace SDK_DYNAMICWORKER_NAMESPACE
