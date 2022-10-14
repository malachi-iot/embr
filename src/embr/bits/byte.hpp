#pragma once

#include "bits.h"
#include "bits-temp.hpp"

#include "detail/byte.hpp"

namespace embr { namespace bits {

// these two setters do not cross byte boundaries.  If that is desired, one must
// cast the 'byte v' to a bigger int which became a paradigm because the underlying
// 'raw' itself must be bigger than a byte for that
// DEBT: Technically we'd still like to pass in a byte here and cross boundaries.  Probably a way to really do
// that would be to tighten up the 'width' parameter, perhaps specializing on it
template <resume_direction rd>
struct setter<no_endian, lsb_to_msb, rd>
{
    // NOTE: Using v3 temporarily as we slowly migrate this 'byte' setter away
    template <class TIt, typename TInt>
    inline static void set(const descriptor d, TIt raw, TInt v)
    {
        bits::experimental::subbyte_setter<lsb_to_msb>::set(d, raw, v);
    }
};

template <resume_direction rd>
struct setter<no_endian, msb_to_lsb, rd>
{
    template <class TIt, typename TInt>
    inline static void set(descriptor d, TIt raw, TInt v)
    {
        bits::experimental::subbyte_setter<lsb_to_msb>::set(
            descriptor{d.bitpos + 1 - d.length, d.length},
            raw, v
        );
    }
};

template <resume_direction rd>
struct getter<no_endian, lsb_to_msb, rd>
{
    template <typename TInt, class TIt>
    inline static TInt get(const descriptor d, TIt raw)
    {
        const TInt mask = (1 << (d.length)) - 1;
        return (*raw >> d.bitpos) & mask;
    }
};


template <resume_direction rd>
struct getter<no_endian, msb_to_lsb, rd>
{
    template <typename TInt, class TIt>
    inline static TInt get(descriptor d, TIt raw)
    {
        return getter<no_endian, lsb_to_msb>::get<TInt>(
            descriptor{d.bitpos + 1 - d.length, d.length}, raw);
    }
};

}}