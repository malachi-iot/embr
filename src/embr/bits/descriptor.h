#pragma once

#include "../internal/bits/base.h"

namespace embr { namespace bits {

struct descriptor : internal::descriptor_base<64>
{
    typedef internal::descriptor_base<64> base_type;

    /// Integer type used to track bit position and bit length
    typedef typename base_type::bitness_type bitness_type;
    /// Integer type of minimum size necessary for descriptor to be valid
    typedef typename base_type::uint_type uint_type;

    // DEBT: Needs a cleanup
    constexpr descriptor(unsigned bitpos, unsigned length) : base_type{(bitness_type)bitpos, (bitness_type)length} {}
    constexpr descriptor(int bitpos, int length) : base_type{(bitness_type)bitpos, (bitness_type)length} {}

    template <size_t bitness>
    constexpr descriptor(const internal::descriptor_base<bitness>& copy_from) :
        base_type(copy_from) {}

    //const unsigned bitpos;      ///< 0-based, inclusive - starting from lsb
    //const unsigned length;      ///< in bits.  @see length_direction
};

}}