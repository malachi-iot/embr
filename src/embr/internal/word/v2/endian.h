#pragma once

#include <estd/bit.h>

#include "enum.h"
#include "fwd.h"

namespace embr { namespace internal {

template <v2::word_options o>
using map_to_endian = estd::integral_constant<
    estd::endian,
    !(o & v2::word_options::endian_mask) ? estd::endian::native :
        o & v2::word_options::big_endian ? estd::endian::big : estd::endian::little>;


template <v2::word_options o>
using is_native_endian = estd::bool_constant<
    (!(o & v2::word_options::endian_mask) ||
     ((o & v2::word_options::endian_mask) == v2::word_options::native))>;

template <v2::word_options o1, v2::word_options o2>
using is_matching_endian = estd::bool_constant<map_to_endian<o1>::value == map_to_endian<o2>::value>;

}}
