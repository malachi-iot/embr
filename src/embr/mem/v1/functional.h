#pragma once

#include <estd/functional.h>

#if FEATURE_STD_TYPE_TRAITS
#include <type_traits>
#include "traits.h"
#endif

#include "concepts.h"
#include "fwd.h"
#include "lock-handle.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// DEBT: Consider renaming to model_ref since it is non-owning (loosely similar to model_base) though _ref
// implies self contained behavior.  Perhaps sparse_model?
template <class F, class Handle,
    template <class, estd::detail::impl::fn_options> class Impl = estd::detail::impl::function_default>
class model;

template <class R, class ...Args, ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool, class F>
typename Pool::handle_type make_model(Pool* pool, F&& f)
{
    using function_type = estd::detail::function<R(Args...)>;
    using model_type = typename function_type::template model<F>;

    return pool->template construct<model_type>(std::forward<F>(f));
}


// Non-owning
template <class R, class ...Args, class Handle, template <class, estd::detail::impl::fn_options> class Impl>
class model<R(Args...), Handle, Impl> : public sparse_handle<
    typename estd::detail::v2::function<R(Args...), Impl>::model_base, Handle>
{
public:
    using function_type = estd::detail::v2::function<R(Args...), Impl>;

protected:
    using model_base = typename function_type::model_base;
    using base_type = sparse_handle<model_base, Handle>;
    using handle_type = Handle;

    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool, class ...Args2>
    static R invoke_ll(estd::false_type, Pool* pool, handle_type h, Args2&&...args)
    {
        auto underlying = static_cast<model_base*>(pool->lock(h));

        R r = underlying->operator()(std::forward<Args2>(args)...);

        pool->unlock(h);

        return r;
    }

    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool, class ...Args2>
    static void invoke_ll(estd::true_type, Pool* pool, handle_type h, Args2&&...args)
    {
        auto underlying = static_cast<model_base*>(pool->lock(h));

        underlying->operator()(std::forward<Args2>(args)...);

        pool->unlock(h);
    }

public:
    explicit constexpr model(const handle_type& handle) : base_type{handle} {}

    template <class Pool, class F>
    constexpr static model make(Pool* pool, F&& f)
    {
        return model(make_model<R, Args...>(pool, std::forward<F>(f)));
    }


    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool, class ...Args2>
    static R invoke(Pool* pool, handle_type h, Args2&&...args)
    {
        return invoke_ll(estd::is_void<R>{}, pool, h, std::forward<Args2>(args)...);
    }
};

template <class F, class Pool>
using nonowning_function = function<F, lock_handle<Pool>>;

// A gc'd function/functor which doesn't itself track Pool*
// This means it is non-owning similar to std::function_ref
template <class F, class Pool,
    template <class, estd::detail::impl::fn_options> class Impl = estd::detail::impl::function_default>
class sparse_function;

template <class R, class ...Args, class Pool,
    template <class, estd::detail::impl::fn_options> class Impl>
class sparse_function<R(Args...), Pool, Impl> : public model<R(Args...), typename Pool::handle_type>
{
    using base_type = model<R(Args...), typename Pool::handle_type>;
    //using typename base_type::handle_type;

public:
    template <class ...Args2>
    constexpr explicit sparse_function(Args2&&...args) : base_type(std::forward<Args2>(args)...) {}

    template <class ...Args2>
    constexpr R invoke(Pool* pool, Args2&&...args)
    {
        return base_type::invoke(pool, base_type::handle_, std::forward<Args2>(args)...);
    }
};

#if FEATURE_STD_TYPE_TRAITS
static_assert(std::is_trivially_move_constructible<sparse_function<void(), detail::handles_traits_uint8>>::value);
#endif

template <class F, class Pool, Pool* pool>
struct innate_traits<mem::function<F, Pool, pool>>
{
    static constexpr bool unique = true;
    static constexpr bool shared = true;

    template <class Pool2, Pool2* pool2 = nullptr>
    using rebind_shared = mem::v1::shared_handle<mem::function<F, Pool2, pool2>, Pool2, pool2>;
};

}}

// DEBT: Consider always making him shared_handle.  Briefly did that, but it occurs that very tight constraint
// environments may have shared counter disabled (see block_6).  Counterpoint is unique_handle behaves much more
// like std::function
template <class R, class ...Args, class Pool, Pool* pool,
    template <class, estd::detail::impl::fn_options> class Impl>
class function<R(Args...), Pool, pool, Impl> : public detail::v1::unique_handle<Pool, pool>
{
    using base_type = detail::v1::unique_handle<Pool, pool>;
    using typename base_type::handle_type;

#if UNIT_TESTING
public:
#endif
    using model = detail::v1::model<R(Args...), handle_type, Impl>;

public:
    constexpr function(Pool* pool2, estd::nullptr_t) :
        base_type(base_type::null, pool2) {}
    constexpr function(estd::nullptr_t) : base_type(base_type::null) {} // NOLINT

    template <class F2>
    constexpr function(Pool* pool2, F2&& f) :
        base_type(model::make(pool2, std::forward<F2>(f)).handle(), pool2)
    {
    }

    template <class F2>
    constexpr explicit function(F2&& f) :   // NOLINT
        base_type(model::make(pool, std::forward<F2>(f)))
    {
        static_assert(pool != nullptr);
    }

    constexpr R operator()(Args&&...args) const
    {
        return model::invoke(base_type::pool_(), base_type::handle_, std::forward<Args>(args)...);
    }
};


}}
