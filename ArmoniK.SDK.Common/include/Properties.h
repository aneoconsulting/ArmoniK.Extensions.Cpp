#ifndef ARMONIK_SDK_PROPERTIES_H
#define ARMONIK_SDK_PROPERTIES_H

#include <armonik/common/utils/IConfiguration.h>
#include "TaskOptions.h"

namespace SDK_COMMON_NAMESPACE{
    struct Properties {
        armonik::api::common::utils::IConfiguration configuration;
        TaskOptions taskOptions;
    };
}




#endif //ARMONIK_SDK_PROPERTIES_H
