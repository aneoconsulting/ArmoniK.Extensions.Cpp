#include "Properties.h"
#include <utility>

ArmoniK::Sdk::Common::Properties::Properties(const IConfiguration &configuration,
                                             ArmoniK::Sdk::Common::TaskOptions taskOptions)
    : configuration(configuration), taskOptions(std::move(taskOptions)) {}
