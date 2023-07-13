#ifndef ARMONIK_SDK_DURATION_H
#define ARMONIK_SDK_DURATION_H

#include <cstdint>

namespace SDK_COMMON_NAMESPACE {
    struct Duration{
        int64_t seconds;
        int32_t nanos;

        Duration(int64_t seconds, int32_t nanos) : seconds(seconds), nanos(nanos) {}
    };
}

#endif //ARMONIK_SDK_DURATION_H
