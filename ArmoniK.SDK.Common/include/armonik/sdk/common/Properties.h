#pragma once

#include "Configuration.h"
#include "TaskOptions.h"

namespace ArmoniK {
namespace Sdk {
namespace Common {
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
} // namespace Common
} // namespace Sdk
} // namespace ArmoniK
