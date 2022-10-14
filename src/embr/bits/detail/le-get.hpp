#pragma once

#include "../bits-temp.hpp"

namespace embr { namespace bits {

namespace internal {

template <typename TInt>
struct get_assister<little_endian, false, TInt, estd::enable_if_t<(sizeof(TInt) > 1)> >
{
    template <class TReverseIt>
    inline static void get_assist(int& i, TReverseIt& raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();

        // DEBT: turn i into unsigned and do an if statement above, rather than
        // forcing a typecast onto byte_width - though making byte_width an actual
        // signed int wouldn't be so bad
        for (; i > (int) byte_width; i -= byte_width)
        {
            --raw;
            v <<= byte_width;
            v |= *raw;
        }
    }
};

struct le_getter_base : getter_tag
{

};

}

namespace detail {

// Full bit boundary version
template <unsigned bitpos, unsigned length>
struct getter<bitpos, length, little_endian, lsb_to_msb, lsb_to_msb,
    enable<internal::is_valid(bitpos, length) &&
           !internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::getter_tag
{
    // DEBT: adjusters are good idea, but doubling up on the width deducer calc plus
    // all the extraneous trivial math is not great
    constexpr static int adjuster()
    {
        return experimental::offset_adjuster_lsb_to_msb<bitpos, length>();
    }

    constexpr static int adjuster(descriptor d)
    {
        return experimental::offset_adjuster_lsb_to_msb(d);
    }

    template <class TReverseIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) > 1), bool> = true>
    inline static void get_assist(int& i, TReverseIt& raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();

        // DEBT: turn i into unsigned and do an if statement above, rather than
        // forcing a typecast onto byte_width - though making byte_width an actual
        // signed int wouldn't be so bad
        for(; i > (int)byte_width; i -= byte_width)
        {
            --raw;
            v <<= byte_width;
            v |= *raw;
        }
    }


    // if TInt is never big enough to do bit shifting in the first place
    // (8-bit, basically) then don't even try.  Technically this is an
    // optimization, since the loop kicks out any actual attempt to bit shift,
    // but we also do this because clang complains at the theoretical
    // possibility of bitshifting an 8 bit value by 8 bits
    template <class TForwardIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) == 1), bool> = true>
    inline static void get_assist(int& i, TForwardIt& raw, TInt& v)
    {

    }

    template <typename TReverseIt, typename TInt>
    static inline void get(descriptor d, TReverseIt raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();
        const unsigned width =
            internal::width_deducer_lsb_to_msb(d);

        unsigned outside_bits = width - d.length;
        unsigned msb_outside_bits = outside_bits - d.bitpos;
        unsigned msb_inside_bits = byte_width - msb_outside_bits;

        byte msb_mask = (1 << msb_inside_bits) - 1;

        v = *raw & msb_mask;

        int i = d.length - msb_inside_bits;

        get_assist(i, raw, v);

        // at this point, 'i' is 'lsb_inside_bits' which should be
        // the same as byte_width - d.bitpos

        // because subbyte version exists elsewhere, we know that at least one
        // additional byte is to be processed
        --raw;

        {
            unsigned lsb_inside_bits = byte_width - d.bitpos;

            v <<= lsb_inside_bits;

            byte temp = *raw >> d.bitpos;

            v |= temp;
        }
    }

    template <typename TReverseIt, typename TInt>
    static inline void get(TReverseIt raw, TInt& v)
    {
        get(descriptor{bitpos, length}, raw, v);
    }
};


/// multi-byte byte boundary version
template <unsigned bitpos, unsigned length, length_direction ld, resume_direction rd>
struct getter<bitpos, length, little_endian, ld, rd,
    enable<internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::le_getter_base
{
    constexpr static int adjuster()
    {
        return internal::offset_adjuster<bitpos, length>();
    }

    constexpr static int adjuster(descriptor d)
    {
        return internal::offset_adjuster(d);
    }

    template <typename TReverseIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) > 1), bool> = true>
    static inline void get_assist(unsigned sz, TReverseIt raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();

        v = (byte) *raw;

        while(--sz)
        {
            v <<= byte_width;
            v |= (byte) *--raw;
        }
    }

    // DEBT: Consolidate this with full bit flavor, since it's a copy/paste
    template <typename TReverseIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) <= 1), bool> = true>
    static inline void get_assist(unsigned, TReverseIt, TInt&)
    {
    }


    template <typename TReverseIt, typename TInt>
    static inline void get(descriptor d, TReverseIt raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();
        unsigned sz = d.length / byte_width;

        return get_assist(sz, raw, v);
    }

    template <typename TReverseIt, typename TInt>
    static inline void get(TReverseIt raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();
        unsigned sz = length / byte_width;

        return get_assist(sz, raw, v);
    }
};


}

}}
