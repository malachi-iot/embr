#pragma once

#include "../bits-temp.hpp"

namespace embr { namespace bits {

namespace detail {

// for subbyte operations, endianness and resume_direction do not matter
template <unsigned bitpos, unsigned length, endianness e, resume_direction rd>
struct setter<bitpos, length, e, lsb_to_msb, rd,
    enable<internal::is_subbyte(bitpos, length)> > :
    internal::setter_tag
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
    enable<internal::is_subbyte(bitpos, length)> > :
    internal::getter_tag
{
    static constexpr int adjuster() { return 0; }
    static constexpr int adjuster(descriptor) { return 0; }

    template <class TIt, typename TInt>
    inline static void get(descriptor d, TIt raw, TInt& v)
    {
        const byte mask = (1 << (d.length)) - 1;
        v = (*raw >> d.bitpos) & mask;
    }

    template <class TIt, typename TInt>
    inline static void get(TIt raw, TInt& v)
    {
        const byte mask = (1 << (length)) - 1;
        v = (*raw >> bitpos) & mask;
    }
};

}

}}
