#pragma once

#include <estd/internal/macros.h>

#include "concepts.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

class block_8;

template <class HandlesTraits, class Block = block_8>
struct bundle_base;

template <class Container>
struct handles_traits;

template <ESTD_CPP_CONCEPT(concepts::HandlesTraits) Traits>
class handles;

template <class Container>
struct pool_traits;

template <class Traits>
class storage;

template <class Traits>
class pool_ops;

template <class T, class PoolTraits, class HandlesTraits, class ...Args>
typename HandlesTraits::handle_type construct(storage<PoolTraits>& p, handles<HandlesTraits>& h, Args&&...args);

}}

}}
