#pragma once

#include "config.h"
#include "enum.h"

namespace embr { namespace dsp { inline namespace v1 {

namespace detail {

#if FEATURE_EMBR_DSP_PRECALC_TABLE_STATIC
extern float sin_table[EMBR_DSP_PRECALC_TABLE_SZ];
#else
extern float* sin_table;
#endif

template <precalc_modes>
struct precalc;

}
    
}}}
