#pragma once

#include "fwd.h"

namespace embr { namespace internal {

// Enough options are starting to appear that perhaps a consolidated "traits" is more tidy
// EXPERIMENTAL
template <unsigned bits_, v2::word_options o, uint32_t padding>
struct word_traits {
    using this_type = word_traits<bits_, o, padding>;
    using ot = v2::word_options;

    static constexpr unsigned bits = bits_;
    static constexpr ot options = o;

    using info = type_from_bits<bits, o & ot::is_signed>;

    static constexpr uint32_t pad = padding;
    static constexpr unsigned lhs_pad = padding & 0xFF00 >> 8;
    static constexpr unsigned rhs_pad = padding & 0xFF;

    static constexpr estd::endian endian = map_to_endian<o>::value;

    static constexpr bool is_array = (o & ot::raw) | (o & ot::packed && !info::matched);

    using int_type = info::type;
    using value_type = embr::detail::v2::word<this_type>;
};

}}