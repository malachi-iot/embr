/**
 * References:
 *
 * 1. README.md
 */
#pragma once

#include "byte.hpp"
#include "bits-temp.hpp"

#include <estd/iterator.h>

#include "detail/le-set.hpp"

#include <embr/platform/guard-in.h>

namespace embr { namespace bits {

namespace internal {

// TODO: width_deducer and friends need documentation

// given an arbitrary int, how many bits (in byte boundaries) are needed to represent
// the value
template <class TInt>
unsigned width_deducer_old(TInt v)
{
    constexpr unsigned byte_width = byte_size();
    constexpr unsigned max_width = sizeof(TInt) * byte_width;
    TInt max_mask = estd::numeric_limits<TInt>::max();
    constexpr unsigned min_width = byte_width;
    constexpr TInt min_mask = (1 << byte_width) - 1;
    TInt mask = ~min_mask;
    TInt width = min_width;

    auto test = v & mask;

    while ((v & mask) != 0)
    {
        mask <<= byte_width;
        width += byte_width;
    }

    return width;
}

// I think this one will optimize out better than the width_deducer_old, but it does
// bear inspection at the assembly level
template <class TInt>
inline unsigned width_deducer(TInt v)
{
    constexpr unsigned byte_width = byte_size();
    constexpr unsigned max_width = sizeof(TInt) * byte_width;
    constexpr TInt max_mask = estd::numeric_limits<TInt>::max();
    constexpr unsigned min_width = byte_width;
    constexpr TInt min_mask = (1 << byte_width) - 1;
    TInt mask = ~min_mask;
    TInt width = min_width;

    auto test = v & mask;

    for (; width < max_width; width += byte_width, mask <<= byte_width)
    {
        if ((v & mask) == 0) break;
    }

    return width;
}

/*
template <class TInt, unsigned width>
constexpr unsigned _width_deducer2(TInt v)
{
    // NOTE: c++14 and high needed for constexpr function variables, even if they
    // too are constexpr
    constexpr unsigned byte_width = byte_size();
    constexpr unsigned max_width = sizeof(TInt) * byte_width;
    constexpr TInt mask = (1 << width) - 1;

    return ((v & mask) == 0) || width >= max_width
        ? width : _width_deducer2<TInt, width + byte_width>(v);
}
*/

template <class TInt, unsigned width,
    estd::enable_if_t<width >= sizeof(TInt) * byte_size(), bool> = true>
constexpr unsigned ___width_deducer2(TInt v)
{
    return width;
}

template <class TInt, unsigned width,
    estd::enable_if_t<width < sizeof(TInt) * byte_size(), bool> = true>
constexpr unsigned ___width_deducer2(TInt v)
{
    //constexpr TInt mask = (1 << width) - 1;

    //return (v & ~mask) == 0 ? width :

    return (v & ~((1 << width) - 1)) == 0 ? width :
           ___width_deducer2<TInt, width + byte_size()>(v);
}


template <class TInt>
constexpr unsigned width_deducer2(TInt v)
{
    return ___width_deducer2<TInt, byte_size()>(v);
}

}

// [1] 2.1.3.1.
template <>
struct setter<endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::lsb_to_msb
    >
{
    // NOTE: Keeping this because calling semantic is rather smooth with this
    // flavor
    template <unsigned bitpos, unsigned length, typename TIt, typename TInt>
    static inline void set(TIt raw, TInt v)
    {
        experimental::set<bitpos, length, little_endian, lsb_to_msb, lsb_to_msb>(raw, v);
    }

    template <typename TIt, typename TInt>
    static inline void set(descriptor d, TIt raw, TInt v)
    {
        // DEBT: Enable or disable these cases with compile time config, possibly enum-flag style

        if(internal::is_subbyte(d))
        {
            typedef experimental::subbyte_setter<lsb_to_msb> s;

            s::set(d, raw + s::adjuster(d), v);
        }
        else if(internal::is_byte_boundary(d))
        {
            typedef experimental::byte_boundary_setter<little_endian> s;

            s::set(d, raw + s::adjuster(d), v);
        }
        else if(internal::is_valid(d))
        {
            typedef detail::setter<1, 8, little_endian, lsb_to_msb> s;

            s::set(d, raw + s::adjuster(d), v);
        }
        else
        {
            // DEBT: Perhaps some kind of error logging?
        }
    }
};


// [1] 2.1.3.2.
template <>
struct setter<endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::msb_to_lsb
>
{
    ///
    /// @tparam TInt
    /// @tparam TForwardIt forward iterator
    /// @param width
    /// @param d
    /// @param raw beginning of the stream, without jumping forward.  In other words, position at the littlest end
    /// @param v
    template <class TForwardIt, typename TInt>
    static inline void set(const unsigned width, descriptor d, TForwardIt raw, TInt v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr unsigned byte_width = 8;

        if(d.bitpos + d.length <= byte_width)
        {
            experimental::subbyte_setter<lsb_to_msb>::set(d, raw, v);
            return;
        }

        unsigned msb_already_shifted = d.bitpos;
        unsigned remaining_to_shift_pre_lsb = width - msb_already_shifted;
        unsigned lsb_outside_bit_material = remaining_to_shift_pre_lsb - d.length;

        const byte_type little_mask = (1 << d.bitpos) - 1;
        v <<= d.bitpos;

        *raw &= little_mask;
        *raw |= (byte_type) v;

        // skip first byte, iterate through and skip last byte too
        for(int i = (width / byte_width) - 1; --i > 0;)
        {
            v >>= 8;
            *++raw = (byte_type) v;
        }

        v >>= 8;
        ++raw;

        const byte_type big_mask = (1 << lsb_outside_bit_material) - 1;

        *raw &= big_mask;
        *raw |= ((byte_type) v) << lsb_outside_bit_material;
    }

    template <class TForwardIt, typename TInt>
    static inline void set(descriptor d, TForwardIt raw, TInt v)
    {
        unsigned width = width_deducer_lsb_to_msb(d);
        set(width, d, raw, v);
    }

    // No descriptor, so no bitpos - use more basic behaviors
    template <class TForwardIt, typename TInt>
    static inline void set(TForwardIt raw, TInt v)
    {
        constexpr unsigned width = sizeof(TInt) * byte_size();

        set(width, descriptor{0U, width}, raw, v);
    }
};

/*
// EXPERIMENTAL
// not sure we want the "detailed" bitsize of embr::word, but rather then general int type width
// during 'set' operations
template <size_t bits>
struct setter<embr::word<bits>, endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::lsb_to_msb
>
{
    typedef embr::word<bits> word_type;

    typedef setter<typename word_type::type,
        endianness::little_endian,
        length_direction::lsb_to_msb,
        resume_direction::lsb_to_msb> core_setter;

    template <class TForwardIt>
    static inline void set(descriptor d, TForwardIt raw, word_type v)
    {
        core_setter::set(word_type::width(), d, raw, v.cvalue());
    }
};
 */

namespace internal {

/// Optimized version without bitmasking - i.e. bitpos = 0/7 length=bit width of int
/// @tparam TInt
/// @tparam TForwardIt
/// @param raw
/// @param v
/// @remarks msb/lsb doesn't matter in this context since we're on byte boundaries
// Not yet used
template <class TInt, class TForwardIt>
inline void set_le(TForwardIt raw, TInt v)
{
    typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
    constexpr unsigned byte_width = 8;
    constexpr unsigned width = sizeof(TInt) * byte_width;

    *raw = (byte_type) v;

    // skip first byte, iterate through the rest
    for (int i = (width / byte_width); --i > 0;)
    {
        v >>= 8;
        *++raw = (byte_type) v;
    }
}

}

// DEBT: Devise a way for unit test to test native and non native flavors

template <>
struct setter<little_endian, no_direction>
{
    template <class TByte, typename TInt>
    static inline void set(TByte* raw, TInt v)
    {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        set_native(raw, v);
#else
        set_le(raw, v);
#endif
    }
};


}}

#include <embr/platform/guard-out.h>
