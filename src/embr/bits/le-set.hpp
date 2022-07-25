/**
 * References:
 *
 * 1. README.md
 */
#pragma once

#include "byte.hpp"

#include <estd/iterator.h>

#include <embr/platform/guard-in.h>

namespace embr { namespace bits {

namespace internal {

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

    while((v & mask) != 0)
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

    for(; width < max_width; width += byte_width, mask <<= byte_width)
    {
        if((v & mask) == 0) break;
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

// [1] 2.1.3.1.
template <class TInt>
struct setter<TInt, endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::lsb_to_msb
    >
{
    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) > 1), bool> = true>
    inline static void set_assist(unsigned& i, TForwardIt& raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = byte_size();

        for(; i > byte_width; i -= byte_width)
        {
            *raw++ = (byte_type) v;
            v >>= byte_width;
        }
    }

    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) == 1), bool> = true>
    inline static void set_assist(unsigned& i, TForwardIt& raw, TInt& v)
    {

    }

    // on bitpos=0 boundary
    // DEBT: It sure seems like the bigger, badder set can handle this without concerningly extra logic
    template <class TForwardIt>
    static void set(const unsigned width, unsigned v_length, TForwardIt raw, TInt v)
    {
        // DEBT: theoretically, byte_type might differ from embr::bits::byte - so
        // check for that
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = byte_size();
        unsigned outside_material = width - v_length;
        unsigned outside_right_material = outside_material;
        unsigned inside_right_material = byte_width - outside_right_material;

        //unsigned v_size = width_deducer2(v);

        byte_type
            // mask for last 'raw', for LE that is loosely MSB
            right_mask = (1 << inside_right_material) - 1;

        set_assist(v_length, raw, v);
        /*
        for(int i = v_length; i > byte_width; i -= byte_width)
        {
            *raw++ = (byte_type) v;
            v >>= 8;
        } */

        // only applicable when v_length is not a multiple of byte_width.  Otherwise
        // it's just a slightly more expensive = assignment
        *raw &= ~right_mask;
        *raw |= v;
    }

    ///
    /// @tparam TForwardIt
    /// @param width bit width, on byte boundaries, of underlying data store for 'raw'
    /// @param d
    /// @param raw
    /// @param v
    /// @remarks (width - d.length) must be <= 8 otherwise behavior is undefined
    template <class TForwardIt>
    static void set(const unsigned width, descriptor d, TForwardIt raw, TInt v)
    {
        constexpr size_t byte_width = byte_size();

        if(d.bitpos + d.length <= byte_width)
        {
            setter<byte, no_endian, lsb_to_msb>::set(d, raw, v);
            return;
        }

        if(d.bitpos == 0)
        {
            set(width, d.length, raw, v);
            return;
        }

        // DEBT: theoretically, byte_type might differ from embr::bits::byte - so
        // check for that
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        unsigned outside_material = width - d.length;
        unsigned outside_right_material = outside_material - d.bitpos;
        unsigned inside_right_material = byte_width - outside_right_material;

        unsigned v_size = width_deducer2(v);

        // NOTE: May actually not be necessary, since trailing zeroes for LE are like leading zeroes
        // for BE
        if(v_size >= d.length)
        {
            // incoming v's value is smaller than length allocated, so we may have to skip some
            // 'raw' bytes first since we're doing this forwards and not in reverse

            // come up with fake leading zeroes somehow
        }

        //size_t shift = 0;
        byte_type
            // mask for first 'raw', for LE that is loosely LSB
            left_mask = (1 << d.bitpos) - 1,
            // mask for last 'raw', for LE that is loosely MSB
            right_mask = (1 << inside_right_material) - 1;

        // first, smallest part of value is written, shifted over by bit position
        *raw &= left_mask;
        *raw |= (byte_type) (v << d.bitpos);

        unsigned inside_left_material = byte_width - d.bitpos;

        v >>= inside_left_material;

        // i = remaining bits to store, now that LSB on left side is written
        // since we check for sub-byte d.length at the top, there is no danger of
        // rollunder
        unsigned i = d.length - inside_left_material;

        set_assist(i, ++raw, v);

        /*
        for(; i > byte_width; i -= byte_width)
        {
            *++raw = (byte_type) v;
            v >>= 8;
        }

        // DEBT: This part feels sloppy
        if(i > 0) ++raw; */

        *raw &= ~right_mask;
        *raw |= v;
    }

    template <class TForwardIt>
    inline static void set(descriptor d, TForwardIt raw, TInt v)
    {
        unsigned width = width_deducer_lsb_to_msb(d);
        return set(width, d, raw, v);
    }

    template <class TForwardIt>
    static inline void set(TForwardIt raw, TInt v)
    {
        constexpr unsigned width = sizeof(TInt) * byte_size();

        set(width, width, raw, v);
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length, class TForwardIt>
    static inline void set(TForwardIt raw, TInt v)
    {
        constexpr unsigned width = width_deducer_lsb_to_msb<bitpos, length>();

        set(width, descriptor{bitpos, length}, raw, v);
    }

    // EXPERIMENTAL
    template <class TReverseIt>
    static void set2(const unsigned width, descriptor d, TReverseIt raw, TInt v)
    {
        typedef typename estd::iterator_traits<TReverseIt>::value_type byte_type;
        constexpr size_t byte_width = byte_size();
        constexpr size_t max_int_width = sizeof(TInt) * byte_width;
        unsigned outside_material = width - d.length;
        unsigned outside_right_material = outside_material - d.bitpos;
        unsigned inside_right_material = byte_width - outside_right_material;

        size_t shifter = max_int_width - byte_width;

        while(shifter > byte_width)
        {
            byte_type _v = v >> shifter;
            shifter -= byte_width;
        }
    }
};


// [1] 2.1.3.2.
template <class TInt>
struct setter<TInt, endianness::little_endian,
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
    template <class TForwardIt>
    static inline void set(const unsigned width, descriptor d, TForwardIt raw, TInt v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr unsigned byte_width = 8;

        if(d.bitpos + d.length <= byte_width)
        {
            setter<byte, no_endian, lsb_to_msb>::set(d, raw, v);
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

    template <class TForwardIt>
    static inline void set(descriptor d, TForwardIt raw, TInt v)
    {
        unsigned width = width_deducer_lsb_to_msb(d);
        set(width, d, raw, v);
    }

    // No descriptor, so no bitpos - use more basic behaviors
    template <class TForwardIt>
    static inline void set(TForwardIt raw, TInt v)
    {
        constexpr unsigned width = sizeof(TInt) * byte_size();

        set(width, descriptor{0U, width}, raw, v);
    }
};


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
    for(int i = (width / byte_width); --i > 0;)
    {
        v >>= 8;
        *++raw = (byte_type) v;
    }
}

// DEBT: Devise a way for unit test to test native and non native flavors

template <typename TInt>
struct setter<TInt, little_endian, no_direction>
{
    template <class TByte>
    static inline void set(TByte* raw, TInt v)
    {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        set_native(raw, v);
#else
        set_le(raw, v);
#endif
    }
};



}

}}

#include <embr/platform/guard-out.h>
