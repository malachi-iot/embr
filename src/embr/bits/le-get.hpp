/**
 * References:
 *
 * 1. Reserved
 * 2. README.md
 */
#pragma once

#include "bits-temp.hpp"
#include "byte.hpp"

#include "detail/le-get.hpp"

namespace embr { namespace bits {

// [2] 2.1.3.1.
template <>
struct getter<endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::lsb_to_msb
    >
{
    // DEBT: Only expose fully-adjusted at this level.  If programmer wants non-adjusted
    // iterator, use the v3 getter directly.  Also rename 'get_adjusted' to 'get'

    template <unsigned bitpos, unsigned length, typename TInt, class TReverseIt>
    static TInt get(TReverseIt raw)
    {
        typedef detail::getter<bitpos, length, little_endian, lsb_to_msb> g;

        TInt v;

        g::get(raw + g::adjuster(), v);

        return v;
    }

    template <typename TInt, class TReverseIt>
    static TInt get(descriptor d, TReverseIt raw, bool adjusted = true)
    {
#if WORKAROUND_EMBR_MAYBE_UNINITIALIZED
        TInt v = TInt();
#else
        TInt v;
#endif

        if(internal::is_subbyte(d))
        {
            experimental::subbyte_getter<lsb_to_msb>::get(d, raw, v);
        }
        else if(internal::is_byte_boundary(d))
        {
            typedef experimental::byte_boundary_getter<little_endian> g;

            raw += adjusted ? g::adjuster(d) : 0;

            g::get(d, raw, v);
        }
        else
        {
            typedef detail::getter<1, 8, little_endian, lsb_to_msb> g;

            g::get(d,
                raw + (adjusted ? g::adjuster(d) : 0),
                v);
        }

        return v;
    }
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
        unsigned width = width_deducer_lsb_to_msb(d);

        constexpr unsigned byte_width = byte_size();

        if(d.bitpos + d.length <= byte_width)
        {
            return getter<no_endian, lsb_to_msb>::get<TInt>(d, raw);
        }

        raw += ((width - 1) / byte_width);

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

namespace internal {

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