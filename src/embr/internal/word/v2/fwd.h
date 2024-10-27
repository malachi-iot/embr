#pragma once

#include "enum.h"

// NOTE: Trouble in paradise with the whole v1/v2 thing - isn't so great when applied to a whole big namespace
namespace embr { namespace v2 {

///
/// @tparam padding - first 16 bits = left/msb padding, second 16 bits = right/lsb padding - EXPERIMENTAL, DORMANT
///
template <size_t bits, word_options o = word_options::native, uint16_t padding = 0>
struct
#ifdef __GNUC__
// "warning: ignoring packed attribute because of unpacked non-POD field"
// https://stackoverflow.com/questions/66687105/remove-ignoring-packed-attribute-because-of-unpacked-non-pod-field-warnings-in
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58798
// Although 100% of 'word' implementations have just one member, without this directive the warning still appears
// when using this in other packed structs
__attribute__((__packed__))
#endif
word;

}}  // embr::v2

namespace embr { namespace internal {

template <size_t bits, v2::word_options o, class enabled = void>
//template <size_t bits, v2::word_options o, bool enabled = false>
struct word_v2_base;

}}
