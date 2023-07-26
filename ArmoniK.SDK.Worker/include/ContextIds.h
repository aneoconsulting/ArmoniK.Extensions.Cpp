#pragma once
#include <string>
#include <functional>
#include "ArmoniKSDKInterface.h"

struct AppId {
  std::string application_name;
  std::string application_version;

  AppId(const std::string &application_name, const std::string &application_version)
    : application_name(application_name),
      application_version(application_version) {
  }

  bool operator==(const AppId &other) const {
    return other.application_name == application_name && other.application_version == application_version;
  }
};

struct ServiceId {
  AppId appId;
  std::string service_name;
  std::string service_namespace;

  ServiceId(const AppId &appId, const std::string &service_name, const std::string &service_namespace) : appId(appId), service_name(service_name), service_namespace(service_namespace){}
  
  bool operator==(const ServiceId &other) const {
    return other.appId == appId && other.service_name == service_name && other.service_namespace == service_namespace;
  }
};

struct ArmoniKFunctionPointers {
  std::function<void *(const char *)> create_service;
  std::function<void(void *)> destroy_service;
  std::function<void *(void *, const char *)> enter_session;
  std::function<void (void *, void *)> leave_session;
  std::function<armonik_status_t (void *, void *, const char *, const char *,
                       size_t , void (*)(armonik_status_t, const char *, size_t ))> call;
};
