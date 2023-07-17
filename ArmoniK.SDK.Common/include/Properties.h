#ifndef ARMONIK_SDK_PROPERTIES_H
#define ARMONIK_SDK_PROPERTIES_H

#include "TaskOptions.h"
#include <armonik/common/utils/IConfiguration.h>

namespace SDK_COMMON_NAMESPACE {
struct Properties {
  Properties(const armonik::api::common::utils::IConfiguration &configuration, TaskOptions taskOptions);

  armonik::api::common::utils::IConfiguration configuration;
  TaskOptions taskOptions;
};
} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_PROPERTIES_H
