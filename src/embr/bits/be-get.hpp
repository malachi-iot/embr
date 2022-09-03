/***
 *
 */
#pragma once

#include "bits.h"
#include "byte.hpp"
#include "bits-temp.hpp"

namespace embr { namespace bits {

namespace internal {

template <class TInt, class TForwardIt>
inline TInt get_be_lsb_to_msb(const unsigned width, descriptor d, TForwardIt raw);

}

namespace experimental {


template <unsigned bitpos, unsigned length>
struct getter<bitpos, length, big_endian, lsb_to_msb, lsb_to_msb,
    enable<is_valid(bitpos, length) &&
            // For the time being, this one serves byte boundary mode also
            // Neat and elegant that one function works that well, but also slightly
            // slower than a distinct function if it's available.  So leans on the "win" side
           //!is_byte_boundary(bitpos, length) &&
           !is_subbyte(bitpos, length)> > :
   getter_tag
{
    constexpr static int adjuster() { return 0; }

    constexpr static int adjuster(descriptor) { return 0; }

    // NOTE: Copy/pasted & adapted from le-get, not fully reworked for BE yet
    template <typename TReverseIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) > 1), bool> = true>
    static inline void get_assist(unsigned sz, TReverseIt raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();

        v = (byte) *raw;

        while(--sz)
        {
            v <<= byte_width;
            v |= (byte) *++raw;
        }
    }

    // Prep for uint8_t operation
    template <typename TForwardIt, typename TInt,
        estd::enable_if_t<(sizeof(TInt) <= 1), bool> = true>
    static inline void get_assist(unsigned, TForwardIt, TInt&)
    {
    }

    template <typename TForwardIt, typename TInt>
    static inline void get(descriptor d, TForwardIt raw, TInt& v)
    {
        const unsigned width = internal::width_deducer_lsb_to_msb(d);

        v = internal::get_be_lsb_to_msb<TInt>(width, d, raw);
    }

    template <typename TForwardIt, typename TInt>
    static inline void get_unready(descriptor d, TForwardIt raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();

        v = *raw;

        v >>= d.bitpos;

        // FIX: Probably this math is wrong
        int i = d.length - d.bitpos;

        get_assist(i, raw, v);

        // FIX: Need to handle remainder byte.  Remember this code generally
        // assumes multibyte operation
    }
};

}

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

// FIX: Either this or the dual lsb_to_msb collide, one needs work
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
        return get(width_deducer_lsb_to_msb(d), d, raw);
    }
};

template <>
struct getter<big_endian, lsb_to_msb, lsb_to_msb>
{
    template <typename TInt, class TForwardIt>
    static inline TInt get(descriptor d, TForwardIt raw)
    {
        const unsigned width = width_deducer_lsb_to_msb(d);

        return internal::get_be_lsb_to_msb<TInt>(width, d, raw);
    }


    template <unsigned bitpos, unsigned length, typename TInt, class TForwardIt>
    static TInt get(TForwardIt raw)
    {
        typedef experimental::getter<bitpos, length, big_endian, lsb_to_msb> g;
        // DEBT: Reliance on descriptor means less compile time optimization opportunity
        // Have to do this right now because v3 getter isn't fully built out
        CONSTEXPR descriptor d(bitpos, length);

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
    template <typename TForwardIterator1, typename TForwardIterator2>
    static bool eval(TForwardIterator1 lhs_start, TForwardIterator2 lhs_end, TForwardIterator2 rhs_start)
    {
        TForwardIterator2 rhs = rhs_start;

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
    template <typename TBase>
    inline static bool eval(
        const embr::bits::internal::provider<endianness::big_endian, TBase>& lhs,
        const embr::bits::internal::provider<endianness::big_endian, TBase>& rhs)
    {
        // DEBT: put in check for differing sizes

        auto lhs_begin = lhs.begin();
        // DEBT: one of the underlying arrays doesn't seem to provide a const end()
        const unsigned char* lhs_end = lhs.end();

        return eval(lhs_begin, lhs_end, rhs.begin());
    }
};

}

}}