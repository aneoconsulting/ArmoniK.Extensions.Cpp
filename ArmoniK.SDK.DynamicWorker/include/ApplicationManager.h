#pragma once

#include "ContextIds.h"
#include "DynamicLib.h"
#include "ServiceManager.h"
#include <Worker/ProcessStatus.h>
#include <armonik/common/logger/Logger.h>
#include <armonik/worker/Worker/TaskHandler.h>

namespace ArmoniK {
namespace Sdk {
namespace Common {
class Configuration;
}
} // namespace Sdk
} // namespace ArmoniK

namespace ArmoniK {
namespace Sdk {
namespace DynamicWorker {
/**
 * @brief Application manager to load and unload applications
 */
class ApplicationManager {

public:
  /**
   * @brief Creates an application manager
   * @param config Configuration
   */
  explicit ApplicationManager(const ArmoniK::Sdk::Common::Configuration &config,
                              const armonik::api::common::logger::Logger &logger);

  /**
   * @brief Configures the application manager to use the given application
   * @param appId Application Id
   * @return this ApplicationHandler
   * @note If the appId is different to the current one, the currently loaded service will be destroyed and the
   * application will be unloaded before loading the new one
   * @note If the appId is identical to the current one, this call does nothing
   */
  ApplicationManager &UseApplication(const AppId &appId) &;

  /**
   * @brief Configures the application manager to use the given service
   * @param serviceId Service id
   * @return this Application manager
   * @note If the current service matches the requested service id, then this does nothing
   * @note If the current service is different from the requested service, then this will destroy the current service
   * before creating the new one
   */
  ApplicationManager &UseService(const ServiceId &serviceId) &;

  /**
   * @brief Configures the applcation manager to use the given session
   * @param sessionId Session id
   * @return this Application manager
   * @note See ArmoniK::Sdk::DynamicWorker::ServiceManager::UseSession() for more info
   */
  ApplicationManager &UseSession(const std::string &sessionId) &;

  /**
   * @brief Executes the task given by the task handler
   * @param taskHandler Task handler
   * @param method_name Name of the method to execute
   * @param method_arguments Serialized arguments of the method
   * @return ProcessStatus telling whether the call was successful or not
   */
  armonik::api::worker::ProcessStatus Execute(armonik::api::worker::TaskHandler &taskHandler,
                                              const std::string &method_name, const std::string &method_arguments);

private:
  /**
   * @brief Loaded application's function pointers
   */
  ArmoniKFunctionPointers functionPointers;
  /**
   * @brief Currently loaded application id
   */
  AppId currentId;

  /**
   * @brief Current service manager
   */
  ServiceManager service_manager;

  /**
   * @brief Currently loaded library
   */
  DynamicLib currentLibrary;

  /**
   * @brief Base path in which to look for the library to load
   */
  std::string applicationsBasePath;

  /**
   * @brief Local Logger
   */
  armonik::api::common::logger::LocalLogger logger;
};
} // namespace DynamicWorker
} // namespace Sdk
} // namespace ArmoniK
