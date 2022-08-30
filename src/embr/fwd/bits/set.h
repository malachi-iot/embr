#pragma once

#include "base.h"

namespace embr { namespace bits {

template <endianness e, class TInt,
#ifdef FEATURE_CPP_DEFAULT_TARGS
    length_direction d = default_direction, resume_direction rd = d,
#else
    length_direction d, resume_direction rd,
#endif
    class TByte>
void set(descriptor, TByte* raw, TInt v);


template <endianness e, length_direction ld, resume_direction rd = ld>
struct setter;

}}