#pragma once

#include <estd/functional.h>

#if FEATURE_STD_TYPE_TRAITS
#include <type_traits>
#include "traits.h"
#endif

#include "fwd.h"
#include "lock-handle.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class F, class Handle>
class model;

template <class R, class ...Args, class Pool, class F>
typename Pool::handle_type make_model(Pool* pool, F&& f)
{
    using function_type = estd::detail::function<R(Args...)>;
    using model_type = typename function_type::template model<F>;

    return pool->template construct<model_type>(std::forward<F>(f));
}


// Non-owning
template <class ...Args, class Handle>
class model<void(Args...), Handle> : public sparse_handle<typename estd::detail::function<void(Args...)>::model_base, Handle>
{
protected:
    using function_type = estd::detail::function<void(Args...)>;
    using model_base = typename function_type::model_base;
    using base_type = sparse_handle<model_base, Handle>;
    using handle_type = Handle;

public:
    explicit constexpr model(const handle_type& handle) : base_type{handle} {}

    template <class Pool, class F>
    constexpr static model make(Pool* pool, F&& f)
    {
        return model(make_model<void, Args...>(pool, std::forward<F>(f)));
    }

    // Remember, paradigm is top-level flattens to Args2...
    // Feels backwards.  See https://github.com/malachi-iot/estdlib/issues/186
    template <class Pool, class ...Args2>
    static void invoke(Pool* pool, handle_type h, Args2&&...args)
    {
        auto underlying = static_cast<model_base*>(pool->lock(h));

        underlying->operator()(std::forward<Args2>(args)...);

        pool->unlock(h);
    }
};


// Non-owning
template <class R, class ...Args, class Handle>
class model<R(Args...), Handle> : public sparse_handle<typename estd::detail::function<R(Args...)>::model_base, Handle>
{
protected:
    using base_type = sparse_handle<typename estd::detail::function<R(Args...)>::model_base, Handle>;
    using function_type = estd::detail::function<R(Args...)>;
    using model_base = typename function_type::model_base;
    using handle_type = Handle;

public:
    explicit constexpr model(const handle_type& handle) : base_type{handle} {}

    template <class Pool, class F>
    constexpr static model make(Pool* pool, F&& f)
    {
        return model(make_model<R, Args...>(pool, std::forward<F>(f)));
    }


    template <class Pool>
    static R invoke(Pool* pool, handle_type h, Args&&...args)
    {
        //typename base_type::template guard<model_base> g{h};

        auto underlying = static_cast<model_base*>(pool->lock(h));

        R r = underlying->operator()(std::forward<Args>(args)...);

        pool->unlock(h);

        return r;
    }
};

template <class F, class Base>
class function;

// DEBT: Should we do CRTP instead?
template <class R, class ...Args, class Base>
class function<R(Args...), Base> : public Base
{
    using base_type = Base;

protected:
    using typename base_type::handle_type;
    using model = detail::v1::model<R(Args...), handle_type>;

public:
    template <class ...Args2>
    constexpr explicit function(Args2&&...args) : base_type(std::forward<Args2>(args)...) {}

    constexpr R operator()(Args&&...args) const
    {
        return model::invoke(base_type::pool_(), base_type::handle_, std::forward<Args>(args)...);
    }
};

template <class F, class Pool>
using nonowning_function = function<F, lock_handle<Pool>>;

// A gc'd function/functor which doesn't itself track Pool*
// This means it is non-owning similar to std::function_ref
template <class F, class Pool>
class sparse_function;

template <class R, class ...Args, class Pool>
class sparse_function<R(Args...), Pool> : public model<R(Args...), typename Pool::handle_type>
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
template <class F, class Pool, Pool* pool>
class function :
    public detail::function<F, detail::v1::unique_handle<Pool, pool>>
{
    using base_type = detail::function<F, detail::v1::unique_handle<Pool, pool>>;
    using typename base_type::model;

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
};


}}
