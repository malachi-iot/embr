/**
 * References:
 *
 * 1. https://github.com/malachib/chariot/tree/main/lib/j1939/j1939 v0.1
 * 2. README.md
 */
#pragma once

#include "bits-temp.hpp"
#include "byte.hpp"

namespace embr { namespace bits {

namespace experimental {

// NOTE: Not 100% sure we need this level of fanciness, I think burying the assisters
// in the getters and setters may be enough
template <endianness e, bool byte_boundary, typename TInt, typename Enabled = void>
struct get_assister;


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
        for(; i > (int)byte_width; i -= byte_width)
        {
            --raw;
            v <<= byte_width;
            v |= *raw;
        }
    }
};


// Full bit boundary version
template <unsigned bitpos, unsigned length>
struct getter<bitpos, length, little_endian, lsb_to_msb, lsb_to_msb,
    enable<is_valid(bitpos, length) &&
           !is_byte_boundary(bitpos, length) &&
           !is_subbyte(bitpos, length)> > :
    getter_tag
{
    // DEBT: adjusters are good idea, but doubling up on the width deducer calc plus
    // all the extraneous trivial math is not great
    constexpr static int adjuster()
    {
        return offset_adjuster_lsb_to_msb<bitpos, length>();
    }

    constexpr static int adjuster(descriptor d)
    {
        return offset_adjuster_lsb_to_msb(d);
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
    enable<is_byte_boundary(bitpos, length) &&
           !is_subbyte(bitpos, length)> > :
    getter_tag
{
    constexpr static int adjuster()
    {
        return offset_adjuster<bitpos, length>();
    }

    constexpr static int adjuster(descriptor d)
    {
        return offset_adjuster(d);
    }

    template <typename TReverseIt, typename TInt>
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

namespace internal {

// [2] 2.1.3.1.
template <>
struct getter<endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::lsb_to_msb
    >
{
#if BITS_LEGACY
    typedef TInt int_type;

    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) > 1), bool> = true>
    inline static void get_assist(int& i, TForwardIt& raw, TInt& v)
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
    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) == 1), bool> = true>
    inline static void get_assist(int& i, TForwardIt& raw, TInt& v)
    {

    }

    template <class TReverseIt>
    static TInt get(const unsigned width, descriptor d, TReverseIt raw)
    {
        constexpr unsigned byte_width = byte_size();

        if(d.bitpos + d.length <= byte_width)
        {
            return getter<byte, no_endian, lsb_to_msb>::get(d, raw);
        }

        unsigned outside_bits = width - d.length;
        unsigned msb_outside_bits = outside_bits - d.bitpos;
        unsigned msb_inside_bits = byte_width - msb_outside_bits;

        byte msb_mask = (1 << msb_inside_bits) - 1;
        
        TInt v = *raw & msb_mask;

        int i = d.length - msb_inside_bits;

        get_assist(i, raw, v);

        /*
        // DEBT: turn i into unsigned and do an if statement above, rather than
        // forcing a typecast onto byte_width - though making byte_width an actual
        // signed int wouldn't be so bad
        for(; i > (int)byte_width; i -= byte_width)
        {
            --raw;
            v <<= 8;
            v |= *raw;
        } */

        // at this point, 'i' is 'lsb_inside_bits' which should be
        // the same as byte_width - d.bitpos

        --raw;

        /*
         * At assembly level, we roughly expect to see:
         * An additional branch/compare in exchange for a subtraction and extra shift
         * It may actually be worth it to still do this, but keeping commented until we
         * actually evaluate the underlying assembly
        if(d.bitpos == 0)
        {
            v <<= byte_width;
            v |= *raw;
        }
        else */
        {
            unsigned lsb_inside_bits = byte_width - d.bitpos;

            v <<= lsb_inside_bits;

            byte temp = *raw >> d.bitpos;

            v |= temp;
        }

        return v;
    }

    template <class TIt>
    inline static TInt get_adjusted(descriptor d, TIt raw)
    {
        unsigned width = width_deducer_lsb_to_msb(d);

        return get(width, d, raw + ((width - 1) / 8));
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length, class TReverseIt>
    static TInt get(TReverseIt raw)
    {
        // TODO: Optimize
        return get(width_deducer_lsb_to_msb<bitpos, length>(), descriptor{bitpos, length}, raw);
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length, class TIt>
    static TInt get_adjusted(TIt raw)
    {
        // TODO: Optimize
        constexpr unsigned width = width_deducer_lsb_to_msb<bitpos, length>();

        return get(width, descriptor{bitpos, length}, raw + ((width - 1) / 8));
    }
#else
    // DEBT: Only expose fully-adjusted at this level.  If programmer wants non-adjusted
    // iterator, use the v3 getter directly.  Also rename 'get_adjusted' to 'get'

    template <unsigned bitpos, unsigned length, typename TInt, class TReverseIt>
    static TInt get_adjusted(TReverseIt raw)
    {
        typedef experimental::getter<bitpos, length, little_endian, lsb_to_msb> g;

        TInt v;

        g::get(raw + g::adjuster(), v);

        return v;
    }

    template <typename TInt, class TReverseIt>
    static TInt get(descriptor d, TReverseIt raw, bool adjusted = true)
    {
        TInt v;

        if(experimental::is_subbyte(d))
        {
            experimental::subbyte_getter<lsb_to_msb>::get(d, raw, v);
        }
        else if(experimental::is_byte_boundary(d))
        {
            typedef experimental::byte_boundary_getter<little_endian> g;

            raw += adjusted ? g::adjuster(d) : 0;

            g::get(d, raw, v);
        }
        else
        {
            typedef experimental::getter<1, 8, little_endian, lsb_to_msb> g;

            g::get(d,
                raw + (adjusted ? g::adjuster(d) : 0),
                v);
        }

        return v;
    }
#endif
};

template <>
struct getter<endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::msb_to_lsb
>
{
    template <typename TInt, class TReverseIt>
    static TInt get(descriptor d, TReverseIt raw)
    {
        bool adjusted = true;
        unsigned width = width_deducer_lsb_to_msb(d);

        constexpr unsigned byte_width = byte_size();

        if(d.bitpos + d.length <= byte_width)
        {
            return getter<no_endian, lsb_to_msb>::get<TInt>(d, raw);
        }

        if(adjusted)
        {
            raw += ((width - 1) / byte_width);
        }

        unsigned msb_already_shifted = d.bitpos;
        unsigned remaining_to_shift_pre_lsb = width - msb_already_shifted;
        unsigned lsb_outside_bit_material = remaining_to_shift_pre_lsb - d.length;

        TInt v = *raw >> lsb_outside_bit_material;

        // skip first byte, iterate through the rest
        for(int i = width / byte_width; --i > 0;)
        {
            v <<= 8;
            v |= *--raw;
        }

        v >>= d.bitpos;

        return v;
    }
};



/*
template <class TInt>
struct getter<TInt, endianness::little_endian,
    length_direction::msb_to_lsb,
    resume_direction::msb_to_lsb
>
{
    template <class TReverseIt>
    static TInt get(const unsigned width, descriptor d, TReverseIt raw)
    {
        // TODO: We did this in old rev1 version, but it doesn't seem a popular use
        // case so not putting in the effort to implement it at the moment
        //return get_le_msb_to_lsb<TInt>(width, d, raw);
        return 0;
    }

    template <class TIt>
    inline static TInt get_adjusted(descriptor d, TIt raw)
    {
        unsigned width = width_deducer_msb_to_lsb(d);

        return get(width, d, raw + ((width - 1) / 8));
    }
};
*/


// FIX: Only keeping this here because actual TInt msb_to_lsb is not yet
// implemented.  This way some minimal non-spanning support for msb_to_lsb
// is present.  Not DEBT because easy to get caught off guard with this
// misleading implementation
template <>
struct getter<endianness::little_endian,
    msb_to_lsb,
    msb_to_lsb
> : getter<no_endian, msb_to_lsb, msb_to_lsb>
{

};


// It is presumed these are the same length, otherwise one is automatically greater than the other
template <bool greater_than, bool or_equal_to, typename TReverseIterator1, typename TReverseIterator2>
bool compare_le(TReverseIterator1 lhs_start, TReverseIterator1 lhs_end, TReverseIterator2 rhs_end)
{
    // do-while because we start one after the end and reverse backward
    do
    {
        --lhs_end;
        --rhs_end;

        if(*lhs_end > *rhs_end)
            return greater_than;
        else if(*lhs_end < *rhs_end)
            return !greater_than;
    }
    while(lhs_start != lhs_end);

    // equals, which is not greater or less than
    return or_equal_to;
}

template <bool greater_than, bool equal_to>
struct compare<endianness::little_endian, greater_than, equal_to>
{
    template <typename TBase>
    inline static bool eval(
        const embr::bits::internal::provider<endianness::little_endian, TBase>& lhs,
        const embr::bits::internal::provider<endianness::little_endian, TBase>& rhs)
    {
        // DEBT: put in check for differing sizes

        auto lhs_begin = lhs.begin();
        // DEBT: one of the underlying arrays doesn't seem to provide a const end()
        const unsigned char* lhs_end = lhs.end();

        return compare_le<greater_than, equal_to>(lhs_begin, lhs_end, rhs.end());
    }
};

template <typename TReverseIterator1, typename TReverseIterator2>
inline bool greater_than_le(TReverseIterator1 lhs_start, TReverseIterator1 lhs_end, TReverseIterator2 rhs_end)
{
    return compare_le<true, false>(lhs_start, lhs_end, rhs_end);
}


}



}}