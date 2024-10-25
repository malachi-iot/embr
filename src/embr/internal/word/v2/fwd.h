#pragma once

#include "enum.h"

// NOTE: Trouble in paradise with the whole v1/v2 thing - isn't so great when applied to a whole big namespace
namespace embr { namespace v2 {

template <size_t bits, word_options o = word_options::native>
struct word;

}}  // embr::v2

namespace embr { namespace internal {

template <size_t bits, v2::word_options o, class enabled = void>
//template <size_t bits, v2::word_options o, bool enabled = false>
struct word_v2_base;

}}
