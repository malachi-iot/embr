#pragma once

#include "../fwd/type_from_bits.h"

#include <estd/cstdint.h>
#include <estd/type_traits.h>

namespace embr {

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits <= 8)> > :
    estd::type_identity<uint8_t>
{
    //static constexpr const int size =
};

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 8 && bits <= 16)> > :
    estd::type_identity<uint16_t>
{ };

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 16 && bits <= 32)> > :
    estd::type_identity<uint32_t>
{ };

template <size_t bits>
struct type_from_bits<bits, false, estd::internal::Range<(bits > 32 && bits <= 64)> > :
    estd::type_identity<uint64_t>
{ };


template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits <= 8)> > :
    estd::type_identity<int8_t>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 8 && bits <= 16)> > :
    estd::type_identity<int16_t>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 16 && bits <= 32)> > :
    estd::type_identity<int32_t>
{ };

template <size_t bits>
struct type_from_bits<bits, true, estd::internal::Range<(bits > 32 && bits <= 64)> > :
    estd::type_identity<int64_t>
{ };


}
