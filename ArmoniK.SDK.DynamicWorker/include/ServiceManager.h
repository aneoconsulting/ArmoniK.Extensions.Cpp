#pragma once

#include "ContextIds.h"
#include <armonik/worker/Worker/ProcessStatus.h>
#include <armonik/worker/Worker/TaskHandler.h>

namespace ArmoniK::Sdk::DynamicWorker {
/**
 * @brief Manager of service for ArmoniK Worker
 */
class ServiceManager {
public:
  /**
   * @brief Empty manager
   */
  ServiceManager() = default;
  /**
   * @brief Manager for the given service
   * @param functionsPointers Dynamic library function pointers
   * @param serviceId Service Id
   */
  ServiceManager(ArmoniKFunctionPointers functionsPointers, ServiceId serviceId);
  ~ServiceManager();

  ServiceManager(const ServiceManager &) = delete;
  ServiceManager &operator=(const ServiceManager &) = delete;

  /**
   * @brief Move constructor
   * @param other Other ServiceManager
   */
  ServiceManager(ServiceManager &&other) noexcept
      : serviceId(std::move(other.serviceId)), current_session(std::move(other.current_session)),
        service_context(other.service_context), session_context(other.session_context),
        functionPointers(other.functionPointers) {
    other.service_context = nullptr;
    other.session_context = nullptr;
    other.serviceId.clear();
  }

  /**
   * @brief Move assignment operator
   * @param other Other ServiceManager
   * @return this
   */
  ServiceManager &operator=(ServiceManager &&other) noexcept {
    using std::swap;
    swap(serviceId, other.serviceId);
    swap(current_session, other.current_session);
    swap(service_context, other.service_context);
    swap(session_context, other.session_context);
    swap(functionPointers, other.functionPointers);
    return *this;
  }

  /**
   * @brief Configure the service to use the given session
   * @param sessionId Session id
   * @return the service manager itself
   * If a different session is in use in the service, it will leave the session before entering the new one.
   * If the current session is the same as the requested session, this call does nothing.
   */
  ServiceManager &UseSession(const std::string &sessionId) &;

  /**
   * @brief Executes a method from the current service, in the current session
   * @param taskHandler ArmoniK task handler
   * @param method_name Name of the method to call
   * @param method_arguments Method's serialized arguments
   * @return Task execution status
   */
  armonik::api::worker::ProcessStatus Execute(armonik::api::worker::TaskHandler &taskHandler,
                                              const std::string &method_name, const std::string &method_arguments);

  /**
   * @brief Checks if the current service matches the given service id
   * @param service_id Service id to check against
   * @return true if the current service matches the one given
   */
  bool matches(const ServiceId &service_id);

  /**
   * @brief Destroys the current service
   */
  void clear();

private:
  /**
   * @brief Current service id
   */
  ServiceId serviceId;
  /**
   * @brief Current session id
   */
  std::string current_session;
  /**
   * @brief Current service context
   */
  void *service_context{};
  /**
   * @brief Current session context
   */
  void *session_context{};
  /**
   * @brief Library function pointers
   */
  ArmoniKFunctionPointers functionPointers{};

  /**
   * @brief Callback for the armonik_call
   * @param opaque_context Context
   * @param status ArmoniK call status
   * @param data Output data or error message
   * @param data_size Output size
   */
  static void UploadResult(void *opaque_context, armonik_status_t status, const char *data, size_t data_size);
};
} // namespace ArmoniK::Sdk::DynamicWorker
