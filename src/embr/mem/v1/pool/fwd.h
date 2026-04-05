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

template <class T, class Traits, class ...Args>
typename pool_ops<Traits>::handle_type construct(pool_ops<Traits>& h, Args&&...args);

template <class Derived>
class pool_crtp;

template <class Derived, class Mutex = void>
class pool_mutex_crtp;

}}

namespace mixins { inline namespace v1 {

template <class Derived>
using pool = detail::pool_crtp<Derived>;

template <class Derived, class Mutex = void>
using pool_mutex = detail::pool_mutex_crtp<Derived, void>;

}}

}}
