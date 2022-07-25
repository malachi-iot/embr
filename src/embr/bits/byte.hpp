#pragma once

#include "bits.h"

namespace embr { namespace bits {

namespace internal {

// these two setters do not cross byte boundaries.  If that is desired, one must
// cast the 'byte v' to a bigger int which became a paradigm because the underlying
// 'raw' itself must be bigger than a byte for that
// DEBT: Technically we'd still like to pass in a byte here and cross boundaries.  Probably a way to really do
// that would be to tighten up the 'width' parameter, perhaps specializing on it
template <resume_direction rd>
struct setter<byte, no_endian, lsb_to_msb, rd>
{
    template <class TIt>
    inline static void set(const descriptor d, TIt raw, byte v)
    {
        // prepare overall value mask
        const byte mask = ((1 << (d.length)) - 1) << d.bitpos;
        // shift it to match position where we're writing it to
        *raw &= ~mask;
        *raw |= v << d.bitpos;
    }
};

template <resume_direction rd>
struct setter<byte, no_endian, msb_to_lsb, rd>
{
    template <class TIt>
    inline static void set(descriptor d, TIt raw, byte v)
    {
        setter<byte, no_endian, lsb_to_msb>::set(
            descriptor{d.bitpos + 1 - d.length, d.length},
            raw, v
        );
    }
};

template <resume_direction rd>
struct getter<byte, no_endian, lsb_to_msb, rd>
{
    template <class TIt>
    inline static byte get(const descriptor d, TIt raw)
    {
        const byte mask = (1 << (d.length)) - 1;
        return (*raw >> d.bitpos) & mask;
    }

    template <class TIt>
    inline static byte get_adjusted(const descriptor d, TIt raw)
    {
        return get(d, raw);
    }
};


template <resume_direction rd>
struct getter<byte, no_endian, msb_to_lsb, rd>
{
    template <class TIt>
    inline static byte get(descriptor d, TIt raw)
    {
        return getter<byte, no_endian, lsb_to_msb>::get(
            descriptor{d.bitpos + 1 - d.length, d.length}, raw);
    }

    template <class TIt>
    inline static byte get_adjusted(descriptor d, TIt raw)
    {
        return get(d, raw);
    }
};

}

}}