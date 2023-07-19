#include "Properties.h"
#include <utility>

ArmoniK::SDK::Common::Properties::Properties(const IConfiguration &configuration,
                                             ArmoniK::SDK::Common::TaskOptions taskOptions)
    : configuration(configuration), taskOptions(std::move(taskOptions)) {}
