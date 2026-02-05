#include <estd/internal/platform.h>

#if ESTD_OS_FREERTOS
#include "../platform/freertos/mem/pool.hpp"
#endif

#include "debounce/reference.h"

namespace embr {

// DEBT: Use flash constant strings
const char* to_string(debounce::v1::States s)
{
    switch(s)
    {
        case debounce::States::On:    return "on";
        case debounce::States::Off:   return "off";
        default:                return "undefined";
    }
}

#if ESTD_OS_FREERTOS && FEATURE_EMBR_GLOBAL_GC
namespace mem { namespace freertos { inline namespace v1 {

// DEBT: Put this guy in a different .cpp
global_pool_type global_pool;

}}}
#endif

}
