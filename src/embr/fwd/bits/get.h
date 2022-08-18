#pragma once

#include "base.h"

namespace embr { namespace bits {

template <endianness e, class TInt, length_direction d = default_direction,
    resume_direction rd = d, class TByte>
TInt get(descriptor, const TByte* raw);


template <endianness e, length_direction ld, resume_direction rd = ld>
struct getter;


}}