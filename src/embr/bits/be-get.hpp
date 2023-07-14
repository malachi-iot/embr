/***
 *
 */
#pragma once

#include "bits.h"
#include "byte.hpp"
#include "bits-temp.hpp"

#include "detail/be-get.hpp"

namespace embr { namespace bits {


namespace internal {

///
/// @tparam TInt - unsigned of some flavor
/// @tparam TIt - iterator of bytes, forward only is all that is required
/// @param width - in bits.  Must be on byte boundary
/// @param d
/// @param raw
/// @return
template <class TInt, class TForwardIt>
inline TInt get_be_lsb_to_msb(const unsigned width, descriptor d, TForwardIt raw)
{
    typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
    constexpr size_t byte_width = sizeof(byte_type) * byte_size();

    TInt v = *raw;

    v >>= d.bitpos;

    // skip first byte, iterate through the rest
    for (int i = width / byte_width; --i > 0;)
    {
        v <<= byte_width;
        v |= *++raw;
    }

    unsigned msb_already_shifted = d.bitpos;
    unsigned remaining_to_shift_pre_lsb = width - msb_already_shifted;
    unsigned lsb_outside_bit_material = remaining_to_shift_pre_lsb - d.length;

    v >>= lsb_outside_bit_material;

    return v;
}

// also has resume_direction of msb_to_lsb
template <class TInt, class TForwardIt>
inline TInt get_be_msb_to_lsb(const unsigned width, descriptor d, TForwardIt raw)
{
    typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
    constexpr size_t byte_width = sizeof(byte_type) * byte_size();
    const TInt mask = (1 << (d.bitpos + 1)) - 1;
    TInt v = *raw & mask;

    // skip first byte, iterate through the rest
    for (int i = width / byte_width; --i > 0;)
    {
        v <<= byte_width;
        v |= *++raw;
    }

    unsigned msb_outside_bit_material = 7 - d.bitpos;
    unsigned lsb_total_bit_material = width - msb_outside_bit_material;
    unsigned lsb_outside_bit_material = lsb_total_bit_material - d.length;

    v >>= lsb_outside_bit_material;

    return v;
}

}

template <>
struct getter<big_endian, lsb_to_msb, msb_to_lsb>
{
private:
    template <typename TInt, class TForwardIt>
    static inline TInt get(unsigned width, descriptor d, TForwardIt raw)
    {
        return internal::get_be_lsb_to_msb<TInt>(width, d, raw);
    }

public:
    template <typename TInt, class TForwardIt>
    static inline TInt get(descriptor d, TForwardIt raw)
    {
        return get<TInt>(width_deducer_lsb_to_msb(d), d, raw);
    }
};

template <>
struct getter<big_endian, lsb_to_msb, lsb_to_msb>
{
    template <unsigned bitpos, unsigned length, typename TInt, class TForwardIt>
    static TInt get(TForwardIt raw)
    {
        typedef detail::getter<bitpos, length, big_endian, lsb_to_msb> g;
        // DEBT: Reliance on descriptor means less compile time optimization opportunity
        // Have to do this right now because v3 getter isn't fully built out
        CONSTEXPR descriptor d(bitpos, length);

        TInt v;

        g::get(d, raw + g::adjuster(), v);

        return v;
    }

    template <typename TInt, class TForwardIt>
    static inline TInt get(descriptor d, TForwardIt raw)
    {
        // DEBT: The "full version" really is full and handles byte boundary OK too, but
        // arguable is not optimal.  Prefer the internal::is_byte_boundary compare both for
        // speed and if we ever change "full version" to NOT support byte boundary flavor
        typedef detail::getter<1, 8, big_endian, lsb_to_msb> g;

        TInt v;

        g::get(d, raw + g::adjuster(), v);

        return v;
    }


};


template <>
struct getter<big_endian, msb_to_lsb, msb_to_lsb>
{
    template <typename TInt, class TForwardIt>
    static inline TInt get(descriptor d, TForwardIt raw)
    {
        unsigned width = width_deducer_msb_to_lsb(d);

        return internal::get_be_msb_to_lsb<TInt>(width, d, raw);
    }
};

namespace internal {

template <bool greater_than, bool equal_to>
struct compare<endianness::big_endian, greater_than, equal_to>
{
    // It is presumed these are the same length, otherwise one is automatically greater than the other
    // DEBT: Once below eval is cleaned up, we can go back to 2 iterator types instead of 3 here
    template <typename TForwardIterator1, typename TForwardIterator2, typename TForwardIterator3>
    static bool eval(TForwardIterator1 lhs_start, TForwardIterator2 lhs_end, TForwardIterator3 rhs_start)
    {
        TForwardIterator3 rhs = rhs_start;

        for(; lhs_start != lhs_end; ++lhs_start, ++rhs)
        {
            if(*lhs_start > *rhs)
                return greater_than;
            else if(*lhs_start < *rhs)
                return !greater_than;

            // equals, which means we need to continue
        }

        // equals, which is not greater or less than
        return equal_to;
    }

    // DEBT: This probably would sit better outside the class
    template <class TBase, class TBase2>
    inline static bool eval(
        const embr::bits::internal::provider<endianness::big_endian, TBase>& lhs,
        const embr::bits::internal::provider<endianness::big_endian, TBase2>& rhs)
    {
        // DEBT: put in check for differing sizes

        // DEBT: Debugger says this isn't const, and yanking iterator out of TBase
        // I can't quite get that to be const properly either.  In either case,
        // get const_iterator equivalents out of lhs and rhs somehow
        auto lhs_begin = lhs.begin();
        // DEBT: one of the underlying arrays doesn't seem to provide a const end()
        const unsigned char* lhs_end = lhs.end();

        return eval(lhs_begin, lhs_end, rhs.begin());
    }
};

}

}}