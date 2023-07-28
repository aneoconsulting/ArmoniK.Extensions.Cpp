#pragma once
#include "ArmoniKSDKInterface.h"
#include <functional>
#include <string>
#include <utility>

namespace SDK_WORKER_NAMESPACE {
struct AppId {
  std::string application_name;
  std::string application_version;

  AppId() = default;

  AppId(std::string application_name, std::string application_version)
      : application_name(std::move(application_name)), application_version(std::move(application_version)) {}

  bool operator==(const AppId &other) const {
    return other.application_name == application_name && other.application_version == application_version;
  }

  bool operator!=(const AppId &other) const { return !(other == *this); }

  [[nodiscard]] bool empty() const { return application_version.empty() && application_name.empty(); }
};

struct ServiceId {
  AppId appId;
  std::string service_name;
  std::string service_namespace;

  ServiceId() = default;

  ServiceId(AppId appId, std::string service_namespace, std::string service_name)
      : appId(std::move(appId)), service_name(std::move(service_name)),
        service_namespace(std::move(service_namespace)) {}

  bool operator==(const ServiceId &other) const {
    return other.appId == appId && other.service_name == service_name && other.service_namespace == service_namespace;
  }

  bool operator!=(const ServiceId &other) const { return !(other == *this); }
};

struct ArmoniKFunctionPointers {
  armonik_create_service_t create_service;
  armonik_destroy_service_t destroy_service;
  armonik_enter_session_t enter_session;
  armonik_leave_session_t leave_session;
  armonik_call_t call;

  ArmoniKFunctionPointers() = default;

  ArmoniKFunctionPointers(armonik_create_service_t createService, armonik_destroy_service_t destroyService,
                          armonik_enter_session_t enterSession, armonik_leave_session_t leaveSession,
                          armonik_call_t call)
      : create_service(createService), destroy_service(destroyService), enter_session(enterSession),
        leave_session(leaveSession), call(call) {}

  void clear() {
    create_service = nullptr;
    destroy_service = nullptr;
    enter_session = nullptr;
    leave_session = nullptr;
    call = nullptr;
  }
};
} // namespace SDK_WORKER_NAMESPACE
