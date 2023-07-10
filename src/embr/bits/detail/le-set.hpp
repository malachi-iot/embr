/**
 * References:
 *
 * 1. Reserved
 * 2. bits/README.md
 */
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
    // DEBT: A lot of minor constexpr opportunities here, optimizations
    // as an artifact of this being a copy/paste.  That said, we quite
    // likely would prefer to retain the more runtime-y behaviors this does
    // too, so maybe 2 versions?
    template <typename TForwardIt, typename TInt>
    static void set(descriptor d, TForwardIt raw, TInt v)
    {
        const unsigned width =
            internal::width_deducer_lsb_to_msb(d);

        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();

        // [2] 2.1.3.1 - total number of bits excluded/not to be changed
        const unsigned excluded = width - d.length;

        // Acquire count of bits excluded on the right side (last byte we write to),
        // For [2] 2.1.3.1 example, that looks like
        // .....101 -> count of all .'s -> 5
        const unsigned outside_right = excluded - d.bitpos;

        // Determine count of bit material on the right side (last byte)
        const unsigned inside_right = byte_width - outside_right;

        const byte_type
        // mask for first 'raw', for LE that is loosely LSB
        left_mask = (1 << d.bitpos) - 1,
        // mask for last 'raw', for LE that is loosely MSB
        right_mask = (1 << inside_right) - 1;

        // First, least significant value bits are adjusted by bitpos and placed
        // into left side (first byte)
        // For [2] 2.1.3.1 example, that looks like
        // ???????? (unknown dest bits) -> 0000xxxx (AND out dest bits) ->
        // 1010xxxx (grab first 4 bits of 1011010 and place it into dest)
        *raw &= left_mask;
        *raw |= (byte_type) (v << d.bitpos);

        // Material present in the first byte (that we just wrote, above)
        const unsigned inside_left = byte_width - d.bitpos;

        // Shift over value by the number of bits we jut applied in first byte above
        // For [2] 2.1.3.1 example, that looks like
        // 1011010 -> 101
        v >>= inside_left;

        // i = remaining bits to store, now that LSB on left side is written
        // since we check for sub-byte d.length at the top, there is no danger of
        // rollunder
        unsigned i = d.length - inside_left;

        // DEBT: When we hit an exact boundary, set_assist takes us one too far.
        // This pulls us down one bit so that on that boundary, we "abort early"
        // Despite being on the right track, this is kludgey thus DEBT
        --i;

        set_assist(i / byte_width, ++raw, v);

        // Final (right) byte
        // For [2] 2.1.3.1 example, that looks like
        // ???????? (unknown dest bits) -> .....000 (AND out space for most significant bits) ->
        // .....101 (OR most significant 3 bits of 1011010)
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