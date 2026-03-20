#pragma once

#include <estd/internal/fwd/dynamic_array.h>

#define EMBR_VECTOR_ADV_ACCESSOR 1

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class T, class Pool, Pool* pool = nullptr>
class vector;

}}
    
inline namespace v1 {

template <class T, class Pool, Pool* pool = nullptr>
using vector = estd::internal::dynamic_array<detail::v1::vector<T, Pool, pool>>;

}}

}
