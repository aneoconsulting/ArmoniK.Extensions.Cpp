#pragma once
#include <ArmoniKSDKInterface.h>
#include <functional>
#include <string>
#include <utility>

namespace SDK_DYNAMICWORKER_NAMESPACE {
/**
 * @brief Id of an application
 */
struct AppId {
  /**
   * @brief Application's name
   */
  std::string application_name;
  /**
   * @brief Application's version
   * @note Can be empty
   */
  std::string application_version;

  /**
   * @brief Equality operator
   * @param other Other AppId
   * @return true if the AppIds' fields are equal
   */
  bool operator==(const AppId &other) const {
    return other.application_name == application_name && other.application_version == application_version;
  }

  /**
   * @brief Inequality operator
   * @param other Other AppId
   * @return false if the AppIds' fields are equal
   */
  bool operator!=(const AppId &other) const { return !(other == *this); }

  /**
   * @brief Checks if the AppId is empty
   * @return True if the application name is empty
   */
  [[nodiscard]] bool empty() const { return application_name.empty(); }

  /**
   * @brief Clears the application name and version
   */
  void clear() {
    application_name.clear();
    application_version.clear();
  }
};

/**
 * @brief Id of a service
 */
struct ServiceId {
  /**
   * @brief Id of the application of the service
   */
  AppId appId;
  /**
   * @brief Name of the service
   * @note Can be empty
   */
  std::string service_name;
  /**
   * @brief Namsepace of the service
   * @note Can be empty
   */
  std::string service_namespace;

  /**
   * @brief Empty service id
   */
  ServiceId() = default;

  /**
   * @brief Creates a service id
   * @param appId Application id of this service
   * @param service_namespace Namespace of the service
   * @param service_name Name of the service
   * @note AppId may not be empty
   * @note Service name and namespace can be empty and still be a valid service id (for use in an application with a
   * single service for example)
   */
  ServiceId(AppId appId, std::string service_namespace, std::string service_name)
      : appId(std::move(appId)), service_name(std::move(service_name)),
        service_namespace(std::move(service_namespace)) {}

  /**
   * @brief Equality operator
   * @param other Other ServiceId
   * @return true if the ServiceIds' fields are equal
   */
  bool operator==(const ServiceId &other) const {
    return other.appId == appId && other.service_name == service_name && other.service_namespace == service_namespace;
  }

  /**
   * @brief Inequality operator
   * @param other Other ServiceId
   * @return false if the ServiceIds' fields are equal
   */
  bool operator!=(const ServiceId &other) const { return !(other == *this); }

  /**
   * @brief Checks if the ServiceId is empty
   * @return True if the application id is empty
   */
  [[nodiscard]] bool empty() const { return appId.empty(); }

  /**
   * @brief Clears service id
   */
  void clear() {
    service_name.clear();
    service_namespace.clear();
    appId.clear();
  }
};

/**
 * @brief Structure containing the dynamic library's function pointers to the \link ArmoniKSDKInterface.h ArmoniK SDK
 * Interface \endlink
 */
struct ArmoniKFunctionPointers {
  /**
   * @brief Function to create a service. See armonik_create_service()
   */
  armonik_create_service_t create_service;
  /**
   * @brief Function to destroy a service. See armonik_destroy_service()
   */
  armonik_destroy_service_t destroy_service;
  /**
   * @brief Function to enter a session. See armonik_enter_session()
   */
  armonik_enter_session_t enter_session;
  /**
   * @brief Function to leave a session. See armonik_leave_session()
   */
  armonik_leave_session_t leave_session;
  /**
   * @brief Function to call a method. See armonik_call()
   */
  armonik_call_t call;

  /**
   * @brief Clears the function pointers
   */
  void clear() {
    create_service = nullptr;
    destroy_service = nullptr;
    enter_session = nullptr;
    leave_session = nullptr;
    call = nullptr;
  }
};
} // namespace SDK_DYNAMICWORKER_NAMESPACE
