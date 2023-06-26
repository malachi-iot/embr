#pragma once

#include "features.h"

#include "../byte.hpp"
#include "../bits-temp.hpp"

namespace embr { namespace bits {

namespace internal {

template <typename TInt>
struct set_assist<little_endian, TInt>
{
    template <typename TForwardIt>
    inline static void set(unsigned i, TForwardIt& raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();
        unsigned sz = i;

        // runtime logic paths in theory can take us here, so doing specialization
        // below
        //static_assert(sizeof(TInt) > 1, "should never get called for byte size int");

        // for(; i > byte_width; i -= byte_width)
        while (sz--)
        {
            *raw++ = (byte_type) v;
            v >>= byte_width;
        }
    }
};

// This shouldn't ever get called, but technically it is callable and
// compiler (rightly) complains with warning about byte_width being bigger
// than int during shift
template <>
struct set_assist<little_endian, byte>
{
    template <class TForwardIt>
#if FEATURE_ESTD_STRICT_BITS_SET
    static void set(unsigned, TForwardIt, byte&)
    {
        abort();
    }
#else
    constexpr static bool set(unsigned, TForwardIt, byte&)
    {
        return {};
    }
#endif
};


// NOTE: Keep this in an internal namespace somewhere
struct le_setter_base : setter_tag
{
    constexpr static int adjuster()
    {
        return 0;
    }

    constexpr static int adjuster(descriptor d)
    {
        return 0;
    }

    template <typename TForwardIt, typename TInt>
    static void set_assist(unsigned i, TForwardIt& raw, TInt& v)
    {
        internal::set_assist<little_endian, TInt>::set(i, raw, v);
    }
};

}

namespace detail {

template <unsigned bitpos, unsigned length>
struct setter<bitpos, length, little_endian, lsb_to_msb, lsb_to_msb,
    enable<internal::is_valid(bitpos, length) &&
           !internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::le_setter_base
{
    // Copy/paste & adapted from internal::setter (v2 version)
    // Passes unit tests, nice
    template <typename TForwardIt, typename TInt>
    static void set(descriptor d, TForwardIt raw, TInt v)
    {
        const unsigned width =
            internal::width_deducer_lsb_to_msb(d);

        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();

        unsigned outside_material = width - d.length;
        unsigned outside_right_material = outside_material - d.bitpos;
        unsigned inside_right_material = byte_width - outside_right_material;

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

        set_assist(i / byte_width, ++raw, v);

        *raw &= ~right_mask;
        *raw |= v;
    }

    template <typename TForwardIt, typename TInt>
    static inline void set(TForwardIt raw, TInt v)
    {
        set(descriptor{bitpos, length}, raw, v);
    }
};

/// multi-byte byte boundary version
template <unsigned bitpos, unsigned length, length_direction ld, resume_direction rd>
struct setter<bitpos, length, little_endian, ld, rd,
    enable<internal::is_byte_boundary(bitpos, length) &&
           !internal::is_subbyte(bitpos, length)> > :
    internal::le_setter_base
{
    template <typename TForwardIt, typename TInt>
    static inline void set(descriptor d, TForwardIt raw, TInt v)
    {
        constexpr unsigned byte_width = byte_size();
        unsigned sz = d.length / byte_width;

        set_assist(sz, raw, v);
    }


    template <typename TForwardIt, typename TInt>
    static inline void set(TForwardIt raw, TInt v)
    {
        constexpr unsigned byte_width = byte_size();
        unsigned sz = length / byte_width;

        set_assist(sz, raw, v);
    }
};

}

}}