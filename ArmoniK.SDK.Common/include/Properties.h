#ifndef ARMONIK_SDK_PROPERTIES_H
#define ARMONIK_SDK_PROPERTIES_H

#include "Configuration.h"
#include "TaskOptions.h"

namespace SDK_COMMON_NAMESPACE {
/**
 * @brief Client properties
 */
struct Properties {
  Properties(const Configuration &configuration, TaskOptions taskOptions);

  /**
   * @brief Configuration
   */
  Configuration configuration;

  /**
   * @brief Default task options
   */
  TaskOptions taskOptions;
};
} // namespace SDK_COMMON_NAMESPACE

#endif // ARMONIK_SDK_PROPERTIES_H
