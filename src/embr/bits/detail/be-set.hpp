#pragma once

#include "fwd.h"
#include "../bits-temp.hpp"

namespace embr { namespace bits {

namespace internal {

template <typename TInt>
struct set_assist<big_endian, TInt>
{
    template <typename TReverseIt>
    static inline void set(unsigned sz, TReverseIt raw, TInt v)
    {
        constexpr unsigned byte_width = byte_size();

        while (sz--)
        {
            *raw-- = (byte) v;
            v >>= byte_width;
        }
    }
};

// In the case of 8 bit integer, we can never shift right by 8
// anyway.  This mainly exists to quiet down compiler bit shift
// warnings for those conditions, but is also potentially a mild
// optimization
template <>
struct set_assist<big_endian, uint8_t>
{
    template <typename TReverseIt>
    static inline void set(unsigned, TReverseIt, uint8_t)
    {

    }
};

}

namespace detail {

// NOTE: Remember, when using this 'enable' trick, specialization has to
// focus inside the 'enable' portion.  For example, "setter<0...." makes
// compiler mad

template <unsigned bitpos, unsigned length>
struct setter<bitpos, length, big_endian, lsb_to_msb, lsb_to_msb,
    enable<internal::is_valid(bitpos, length) &&
           !internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::setter_tag
{
    constexpr static int adjuster()
    {
        return 0;
    }

    // disabling to avoid false positives on compile - keeping overall specialization
    // to further put 'enable' through its paces
    /*
    template <typename TIt, typename TInt>
    static inline void set(TIt raw, TInt v)
    {
    } */
};

// byte boundary flavor means no bitpos/length processing required, so length_direction
// and resume_direction are ignored
template <unsigned bitpos, unsigned length, length_direction ld, resume_direction rd>
struct setter<bitpos, length, big_endian, ld, rd,
    enable<internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::setter_tag
{
    constexpr static int adjuster()
    {
        return internal::offset_adjuster<bitpos, length>();
    }

    constexpr static int adjuster(descriptor d)
    {
        return internal::offset_adjuster(d);
    }

    template <typename TReverseIt, typename TInt>
    static inline void set(descriptor d, TReverseIt raw, TInt v)
    {
        constexpr unsigned byte_width = byte_size();
        unsigned sz = d.length / byte_width;

        internal::set_assist<big_endian, TInt>::set(sz, raw, v);
    }


    template <typename TReverseIt, typename TInt>
    static inline void set(TReverseIt raw, TInt v)
    {
        constexpr unsigned byte_width = byte_size();
        constexpr unsigned _sz = length / byte_width;
        unsigned sz = _sz;

        internal::set_assist<big_endian, TInt>::set(sz, raw, v);
    }
};

}

}}
