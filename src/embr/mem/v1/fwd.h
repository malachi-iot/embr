#pragma once

#if __cpp_concepts
#include <concepts>
#endif

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Container, class Traits = void>
class handles;

template <class Container, class Traits = void>
class pool;

template <class Pool, Pool* pool>
class shared_handle;

}}

namespace internal { inline namespace v1 {

struct bundle;

struct block;

}}


inline namespace v1 {

template <class T, class Pool, Pool* pool = {}>
class shared_handle;


}


}}
