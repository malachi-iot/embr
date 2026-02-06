#pragma once

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Container>
struct handles_traits;

template <class Traits>
class handles;

template <class Container>
struct pool_traits;

template <class Traits>
class pool;

template <class T, class PoolTraits, class HandlesTraits, class ...Args>
typename HandlesTraits::size_type construct(pool<PoolTraits>& p, handles<HandlesTraits>& h, Args&&...args);

}}

}}
