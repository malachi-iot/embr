#pragma once

// 27OCT24 MB Perhaps this guy would be better suited in some fwd somewhere?

#include <estd/internal/raw/type_traits.h>  // Low dependency type_traits suitable for fwd

namespace embr { namespace internal {

// Indicates whether L can 100% safely cast to R
template <class L, class R, class Enabled = void>
struct can_cast : estd::bool_constant<false> {};

}}
