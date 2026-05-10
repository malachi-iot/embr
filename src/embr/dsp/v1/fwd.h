#pragma once

#include <estd/type_traits.h>
#include <estd/utility.h>

#include "enum.h"

namespace embr { namespace dsp {

namespace detail { inline namespace v1 {

template <packed_word_options o, typename BitsSequence, typename Enabled = void>
class packed_word_base;

}}

    
inline namespace v1 {

template <unsigned exponent, unsigned mantissa, fixed_point_options o = FP_DEFAULT, estd::endian e = estd::endian::native>
struct fixed_point;

template <unsigned ...channel_bits>
using packed_word_default = detail::packed_word_base<PACKED_WORD_DEFAULT, estd::integer_sequence<unsigned, channel_bits...>>;

template <packed_word_options o, unsigned ...channel_bits>
using packed_word = detail::packed_word_base<o, estd::integer_sequence<unsigned, channel_bits...>>;

template <class>
struct is_fixed_point : estd::false_type {};

template <unsigned exp, unsigned man, fixed_point_options o>
struct is_fixed_point<fixed_point<exp, man, o>> : estd::true_type {};


}

}}
