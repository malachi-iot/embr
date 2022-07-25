#pragma once

#include "bits.h"

namespace embr { namespace bits {

namespace internal {

template <unsigned bitpos, unsigned length, unsigned byte_width = byte_size()>
constexpr unsigned width_deducer_lsb_to_msb()
{
    return ((length + bitpos) + byte_width - 1) / byte_width * byte_width;
}

inline unsigned width_deducer_lsb_to_msb(descriptor d)
{
    constexpr unsigned byte_width = byte_size();

    // DEBT: Clunky 'ceil'
    unsigned width = (d.length + d.bitpos);
    // NOTE: If width <= 8 at this point, this is a non-spanning, non-endian single byte
    // operation
    width = (width + byte_width - 1) / byte_width * byte_width;

    return width;
}

inline unsigned width_deducer_msb_to_lsb(descriptor d)
{
    constexpr unsigned byte_width = byte_size();

    // DEBT: Clunky 'ceil'
    unsigned width = (d.length + (7 - d.bitpos));
    // NOTE: If width <= 8 at this point, this is a non-spanning, non-endian single byte
    // operation
    width = (width + byte_width - 1) / byte_width * byte_width;

    return width;
}


}

}}