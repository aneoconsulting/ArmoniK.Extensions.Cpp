#include "Properties.h"
#include <utility>

ArmoniK::SDK::Common::Properties::Properties(const armonik::api::common::utils::IConfiguration &configuration,
                                             ArmoniK::SDK::Common::TaskOptions taskOptions)
    : configuration(configuration), taskOptions(std::move(taskOptions)) {}
