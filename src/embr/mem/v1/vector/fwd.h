#pragma once

#include <estd/internal/fwd/dynamic_array.h>

#define EMBR_VECTOR_ADV_ACCESSOR 1

namespace embr { namespace mem {

namespace detail { namespace v1 {

// TODO
template <class T, class Pool, Pool* pool = nullptr>
class vector_impl;

}}
    
inline namespace v1 {

template <class T, class Pool, Pool* pool = nullptr>
class vector_impl;

template <class T, class Pool, Pool* pool = nullptr>
using vector = estd::internal::dynamic_array<vector_impl<T, Pool, pool>>;

}}

}
