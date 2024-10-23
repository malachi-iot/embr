#pragma once

#include "../fwd/type_from_bits.h"

#include <estd/cstdint.h>
#include <estd/type_traits.h>

namespace embr {

namespace internal {

constexpr size_t alias_up(size_t v, size_t alias)
{
    // Seems like there may be something more we can do with this guy
    //return (alias - (v % alias)) + (v % alias);
    return (((v - 1) + alias) / alias) * alias;
}

// UNTESTED
constexpr size_t alias_shift_up(size_t v, size_t alias_bits)
{
    return (((v - 1) + (1 << alias_bits)) >> alias_bits) << alias_bits;
}

template <size_t bits_>
struct type_from_bits_base
{
    static constexpr const unsigned bits = bits_;
    static constexpr const unsigned size = internal::alias_up(bits, 8) / 8;
};

}

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits <= 8)> > :
    estd::type_identity<uint8_t>,
    internal::type_from_bits_base<bits>
{
};

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 8 && bits <= 16)> > :
    estd::type_identity<uint16_t>,
    internal::type_from_bits_base<bits>
{ };

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 16 && bits <= 32)> > :
    estd::type_identity<uint32_t>,
    internal::type_from_bits_base<bits>
{ };

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 32 && bits <= 64)> > :
    estd::type_identity<uint64_t>,
    internal::type_from_bits_base<bits>
{ };


template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits <= 8)> > :
    estd::type_identity<int8_t>,
    internal::type_from_bits_base<bits>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 8 && bits <= 16)> > :
    estd::type_identity<int16_t>,
    internal::type_from_bits_base<bits>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 16 && bits <= 32)> > :
    estd::type_identity<int32_t>,
    internal::type_from_bits_base<bits>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 32 && bits <= 64)> > :
    estd::type_identity<int64_t>,
    internal::type_from_bits_base<bits>
{ };


}
