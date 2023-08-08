#pragma once

#include "Configuration.h"
#include "TaskOptions.h"

namespace SDK_COMMON_NAMESPACE {
/**
 * @brief Client properties
 */
struct Properties {
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
