#pragma once

#include "bits.h"

namespace embr { namespace bits {

namespace internal {

/// Given a bit position and length, indicate how many bits total on byte
/// boundaries are necessary to fully contain the indicated word
template <unsigned bitpos, unsigned length, unsigned byte_width = byte_size()>
constexpr unsigned width_deducer_lsb_to_msb()
{
    return ((length + bitpos) + byte_width - 1) / byte_width * byte_width;
}

/// Given a descriptor, indicate how many bits total on byte boundaries
/// are necessary to fully contain the indicated word
/// @param d
/// @return
inline unsigned width_deducer_lsb_to_msb(descriptor d)
{
    constexpr unsigned byte_width = byte_size();

    // DEBT: Clunky 'ceil'
    unsigned width = (d.length + d.bitpos);
    // NOTE: If width <= 8 at this point, this is a non-spanning, non-endian single byte
    // operation
    width = (width + byte_width - 1) / byte_width * byte_width;

    return width;
}

inline unsigned width_deducer_msb_to_lsb(descriptor d)
{
    constexpr unsigned byte_width = byte_size();

    // DEBT: Clunky 'ceil'
    unsigned width = (d.length + (7 - d.bitpos));
    // NOTE: If width <= 8 at this point, this is a non-spanning, non-endian single byte
    // operation
    width = (width + byte_width - 1) / byte_width * byte_width;

    return width;
}


}

namespace experimental {

// these hold true no matter the endianness or underlying material size
// DEBT: These are good to go for at least 'internal' namespace, their presence
// even in the odd chance we don't use them fits there neatly

constexpr unsigned max_bits()
{
    return byte_size();
}

constexpr bool is_valid(unsigned bitpos, unsigned length)
{
    return bitpos <= max_bits() && length > 0;
}

constexpr bool is_byte_boundary(unsigned bitpos, unsigned length)
{
    return bitpos == 0 && (length % byte_size() == 0);
}

constexpr bool is_subbyte(unsigned bitpos, unsigned length)
{
    return //(bitpos <= max_bits()) &&  // implicit in following line
        (length + bitpos <= 8);
}


constexpr bool is_valid(descriptor d)
{
    return is_valid(d.bitpos, d.length);
}

constexpr bool is_byte_boundary(descriptor d)
{
    return is_byte_boundary(d.bitpos, d.length);
}

constexpr bool is_subbyte(descriptor d)
{
    return is_subbyte(d.bitpos, d.length);
}

// DEBT: Poor naming.  Use this for scenarios which require reverse iteration
// this one is for byte boundaries
template <unsigned bitpos, unsigned length>
constexpr unsigned offset_adjuster()
{
    return (length / byte_size()) - 1;
}

// DEBT: Poor naming.  Use this for scenarios which require reverse iteration
// this one is for byte boundaries
constexpr unsigned offset_adjuster(descriptor d)
{
    return (d.length / byte_size()) - 1;
}

// DEBT: Poor naming.  Use this for scenarios which require reverse iteration
// this one is for 'full' mode
template <unsigned bitpos, unsigned length>
constexpr unsigned offset_adjuster_lsb_to_msb()
{
    return (internal::width_deducer_lsb_to_msb<bitpos, length>() / byte_size()) - 1;
}

// DEBT: Poor naming.  Use this for scenarios which require reverse iteration
// this one is for 'full' mode
inline unsigned offset_adjuster_lsb_to_msb(descriptor d)
{
    return (internal::width_deducer_lsb_to_msb(d) / byte_size()) - 1;
}

struct getter_tag {};
struct setter_tag {};


// for subbyte operations, endianness and resume_direction do not matter
template <unsigned bitpos, unsigned length, endianness e, length_direction ld, resume_direction rd>
struct setter<bitpos, length, e, ld, rd,
    enable<is_subbyte(bitpos, length)> > :
    setter_tag
{
    constexpr static int adjuster()
    {
        return 0;
    }

    constexpr static int adjuster(descriptor d)
    {
        return 0;
    }


    template <typename TIt, typename TInt>
    static inline void set(descriptor d, TIt raw, TInt v)
    {
        bits::set<no_endian, byte, ld>(d, raw, v);
    }

    template <typename TIt, typename TInt>
    static inline void set(TIt raw, TInt v)
    {
        // DEBT: Set up a bits::set with template values
        bits::set<no_endian, byte, ld>(
            descriptor{bitpos, length}, raw, v);
    }
};

template <endianness e, length_direction ld, resume_direction rd = ld>
using subbyte_setter = setter<0, byte_size(), e, ld, rd>;

template <endianness e, length_direction ld>
using subbyte_getter = getter<0, byte_size(), e, ld>;

// byte_boundary_xxxx undefined behavior if TInt is either:
// - not an actual integer type
// - less than 2 bytes big
// DEBT: Feed in an enable/disable if possible to guard against that at compile time
// beware though that doing this will break current runtime-dispatching get/set since
// they are able to accept bytes as well

template <endianness e, class TInt>
using byte_boundary_setter = setter<0, sizeof(TInt) * byte_size(), e, no_direction>;

template <endianness e, class TInt>
using byte_boundary_getter = getter<0, sizeof(TInt) * byte_size(), e, no_direction>;


}

}}