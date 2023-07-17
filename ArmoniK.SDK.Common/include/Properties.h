#ifndef ARMONIK_SDK_PROPERTIES_H
#define ARMONIK_SDK_PROPERTIES_H

#include "TaskOptions.h"
#include <armonik/common/utils/IConfiguration.h>

namespace SDK_COMMON_NAMESPACE {
/**
 * @brief Client properties
 */
struct Properties {
  Properties(const armonik::api::common::utils::IConfiguration &configuration, TaskOptions taskOptions);

  /**
   * @brief Configuration
   */
  armonik::api::common::utils::IConfiguration configuration;

  /**
   * @brief Default task options
   */
  TaskOptions taskOptions;
};
} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_PROPERTIES_H
