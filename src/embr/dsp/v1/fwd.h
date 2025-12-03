#pragma once

#include "enum.h"

namespace embr { namespace dsp { inline namespace v1 {

template <unsigned exponent, unsigned mantissa, fixed_point_options o = FP_DEFAULT, estd::endian e = estd::endian::native>
struct fixed_point;


}}}
