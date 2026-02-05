#pragma once

#include <estd/functional.h>

#include "fwd.h"

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


template <class ...Args, class Handle>
class model<void(Args...), Handle>
{
protected:
    using function_type = estd::detail::function<void(Args...)>;
    using model_base = typename function_type::model_base;
    using handle_type = Handle;

public:
    //model(const model& copy_from) : base_type{copy_from} {}

    template <class Pool, class F>
    constexpr static handle_type make(Pool* pool, F&& f)
    {
        return make_model<void, Args...>(pool, std::forward<F>(f));
    }

    template <class Pool>
    static void invoke(Pool* pool, handle_type h, Args&&...args)
    {
        auto underlying = (model_base*) pool->lock(h);

        underlying->operator()(std::forward<Args>(args)...);

        pool->unlock(h);
    }
};



template <class R, class ...Args, class Handle>
class model<R(Args...), Handle>
{
protected:
    using function_type = estd::detail::function<R(Args...)>;
    using model_base = typename function_type::model_base;
    using handle_type = Handle;

public:
    template <class Pool, class F>
    constexpr static handle_type make(Pool* pool, F&& f)
    {
        return make_model<R, Args...>(pool, std::forward<F>(f));
    }


    // FIX: Not resilient to void return
    template <class Pool>
    static R invoke(Pool* pool, handle_type h, Args&&...args)
    {
        //typename base_type::template guard<model_base> g{h};

        auto underlying = (model_base*) pool->lock(h);

        R r = underlying->operator()(std::forward<Args>(args)...);

        pool->unlock(h);

        return r;
    }
};


}}

// DEBT: Consider always making him shared_handle.  Briefly did that, but it occurrs that very tight constraint
// environments may have shared counter disabled (see block_6).  Counterpoint is unique_handle behaves much more
// like std::function
template <class R, class ...Args, class Pool, Pool* pool>
class function<R(Args...), Pool, pool> : public detail::v1::unique_handle<Pool, pool>
{
    using base_type = detail::v1::unique_handle<Pool, pool>;
    using handle_type = typename Pool::handle_type;
    using model = detail::v1::model<R(Args...), handle_type>;

public:
    constexpr function(Pool* pool2, estd::nullptr_t) :
        base_type(base_type::null, pool2) {}
    constexpr function(estd::nullptr_t) : base_type(base_type::null) {}

    template <class F>
    function(Pool* pool2, F&& f) :
        base_type(model::make(pool2, std::forward<F>(f)), pool2)
    {

    }

    template <class F>
    function(F&& f) :
        base_type(model::make(pool, std::forward<F>(f)))
    {
        static_assert(pool != nullptr);
    }


    constexpr R operator()(Args&&...args)
    {
        return model::invoke(base_type::pool_(), base_type::handle_, std::forward<Args>(args)...);
    }
};


}}
