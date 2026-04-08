#pragma once

#include <estd/functional.h>

#include "../../../internal/mutex.h"

#include "../concepts.h"
#include "../pool/fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class F, class Handle,
    template <class, estd::detail::impl::fn_options> class Impl = estd::detail::impl::function_default>
class sparse_model;

// A gc'd function/functor which doesn't itself track Pool*
// This means it is non-owning similar to std::function_ref
template <class F, ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool,
    template <class, estd::detail::impl::fn_options> class Impl = estd::detail::impl::function_default>
class sparse_function;

}}

inline namespace v1 {

template <class F, class Pool, Pool* pool = nullptr>
class funclist;

template <class Traits, class It, class End, class Mutex = embr::internal::noop_mutex>
void multi_lock(detail::v1::pool_ops<Traits>& ops, It begin, End end, Mutex mutex = {});

template <class Traits, class It, class End, class Mutex = embr::internal::noop_mutex>
void multi_unlock(detail::v1::pool_ops<Traits>& ops, It begin, End end, Mutex mutex = {});

}}} // embr::mem::inline v1
