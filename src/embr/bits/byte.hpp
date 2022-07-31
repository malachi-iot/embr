#pragma once

#include "bits.h"
#include "bits-temp.hpp"

namespace embr { namespace bits {

namespace experimental {

// for subbyte operations, endianness and resume_direction do not matter
template <unsigned bitpos, unsigned length, endianness e, resume_direction rd>
struct setter<bitpos, length, e, lsb_to_msb, rd,
    enable<is_subbyte(bitpos, length)> > :
    setter_tag
{
    constexpr static int adjuster()
    {
        return 0;
    }

    constexpr static int adjuster(descriptor d)
    {
        return 0;
    }


    template <typename TIt, typename TInt>
    static inline void set(descriptor d, TIt raw, TInt v)
    {
        // prepare overall value mask
        const byte mask = ((1 << (d.length)) - 1) << d.bitpos;
        // shift it to match position where we're writing it to
        *raw &= ~mask;
        *raw |= v << d.bitpos;
    }

    template <typename TIt, typename TInt>
    static inline void set(TIt raw, TInt v)
    {
        // DEBT: Set up version with direct template values
        set(descriptor{bitpos, length}, raw, v);
    }
};

template <unsigned bitpos, unsigned length, endianness e, resume_direction rd>
struct getter<bitpos, length, e, lsb_to_msb, rd,
    enable<is_subbyte(bitpos, length)> > :
    getter_tag
{
    static constexpr int adjuster() { return 0; }
    static constexpr int adjuster(descriptor) { return 0; }

    template <class TIt, typename TInt>
    inline static void get(descriptor d, TIt raw, TInt& v)
    {
        const byte mask = (1 << (d.length)) - 1;
        v = (*raw >> d.bitpos) & mask;
    }
};

}

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