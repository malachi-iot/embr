#pragma once

#include <estd/internal/deduce_fixed_size.h>

namespace embr {

/// Deduces an integer/word type based on presented bit count
/// @tparam bits
/// @tparam is_signed
/// @remarks uses estd::type_identity
template <size_t bits, bool is_signed, typename = estd::internal::Range<true> >
struct type_from_bits;

template <size_t bits, bool is_signed>
using type_from_bits_t = typename type_from_bits<bits, is_signed>::type;

}
