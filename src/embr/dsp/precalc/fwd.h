#pragma once

#include "config.h"
#include "enum.h"

namespace embr { namespace dsp { inline namespace v1 {

namespace detail {

extern float sin_table[EMBR_DSP_PRECALC_TABLE_SZ];

template <precalc_modes>
struct precalc;

}
    
}}}
