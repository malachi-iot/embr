#pragma once

#include <estd/functional.h>

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


}}} // embr::mem::inline v1
