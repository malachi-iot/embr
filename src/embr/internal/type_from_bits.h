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

template <class T, size_t bits_>
struct type_from_bits_base : estd::type_identity<T>
{
    static constexpr const unsigned bits = bits_;
    static constexpr const unsigned size = internal::alias_up(bits, 8) / 8;
    static constexpr const bool matched = bits / 8 == sizeof(T);
};

}

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits <= 8)> > :
    internal::type_from_bits_base<uint8_t, bits>
{
};

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 8 && bits <= 16)> > :
    internal::type_from_bits_base<uint16_t, bits>
{ };

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 16 && bits <= 32)> > :
    internal::type_from_bits_base<uint32_t, bits>
{ };

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 32 && bits <= 64)> > :
    internal::type_from_bits_base<uint64_t, bits>
{ };

#if __SIZEOF_INT128__
template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 64 && bits <= 128)> > :
    internal::type_from_bits_base<__uint128_t, bits>
{ };
#endif


template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits <= 8)> > :
    internal::type_from_bits_base<int8_t, bits>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 8 && bits <= 16)> > :
    internal::type_from_bits_base<int16_t, bits>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 16 && bits <= 32)> > :
    internal::type_from_bits_base<int32_t, bits>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 32 && bits <= 64)> > :
    internal::type_from_bits_base<int64_t, bits>
{ };

#if __SIZEOF_INT128__
template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 64 && bits <= 128)> > :
    internal::type_from_bits_base<__int128_t, bits>
{ };
#endif

template <size_t bits, bool is_signed>
struct type_from_bits<bits, is_signed, estd::internal::Range<(bits > 128)> > :
    internal::type_from_bits_base<estd::monostate, bits>
{ };


}
