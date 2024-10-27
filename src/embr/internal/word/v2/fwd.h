#pragma once

#include <estd/internal/raw/type_traits.h>  // Low dependency type_traits suitable for fwd

#include "enum.h"
#include "feature.h"

// "warning: ignoring packed attribute because of unpacked non-POD field"
// https://stackoverflow.com/questions/66687105/remove-ignoring-packed-attribute-because-of-unpacked-non-pod-field-warnings-in
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58798
// Although 100% of 'word' implementations have just one member, without this directive the warning still appears
// when using this in other packed structs
#ifdef __GNUC__
#define GCC_58798_WORKAROUND    __attribute__((__packed__))
#endif

namespace embr { namespace internal {

template <size_t bits, v2::word_options o, class enabled = void>
//template <size_t bits, v2::word_options o, bool enabled = false>
struct GCC_58798_WORKAROUND word_v2_base;

template <size_t bits, v2::word_options o, class Enabled = void>
struct GCC_58798_WORKAROUND word_v2_layer;

// Indicates whether L can 100% safely cast to R
template <class L, class R, class Enabled = void>
struct can_cast : estd::bool_constant<false> {};

}}

// NOTE: Trouble in paradise with the whole v1/v2 thing - isn't so great when applied to a whole big namespace
namespace embr { namespace v2 {

///
/// @tparam padding - first 16 bits = left/msb padding, second 16 bits = right/lsb padding - EXPERIMENTAL, DORMANT
///
template <size_t bits, word_options o = word_options::none, uint16_t padding = 0>
#if FEATURE_EMBR_WORD_ALIAS
using word = internal::word_v2_layer<bits, o>;
#else
struct GCC_58798_WORKAROUND word;
#endif

}}  // embr::v2
