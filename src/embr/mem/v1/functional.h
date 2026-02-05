#pragma once

#include <estd/functional.h>

#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class F, class Pool, Pool* pool = nullptr>
class model;

// DEBT: Consider making model just track the handle, letting 'function' be the one who tracks pool too
template <class R, class ...Args, class Pool, Pool* pool>
class model<R(Args...), Pool, pool> :
    public mem::detail::v1::lock_handle<Pool, pool>
{
    using lock_handle = mem::detail::v1::lock_handle<Pool, pool>;
    using base_type = lock_handle;

protected:
    using function_type = estd::detail::function<R(Args...)>;
    using model_base = typename function_type::model_base;
    using base_type::pool_;
    using typename base_type::handle_type;

public:
    constexpr explicit model(handle_type h = base_type::null, Pool* p = nullptr) : base_type(h, p)   {}

    //model(const model& copy_from) : base_type{copy_from} {}

    template <class F>
    static handle_type make(Pool* pool2, F&& f)
    {
        using model_type = typename function_type::template model<F>;

        return pool2->template construct<model_type>(std::forward<F>(f));
    }

    static R invoke(lock_handle h, Args&&...args)
    {
        //typename base_type::template guard<model_base> g{h};

        auto underlying = (model_base*) h.lock();

        R r = underlying->operator()(std::forward<Args>(args)...);

        h.unlock();

        return r;
    }

    friend R invoke(model& m, Args&&...args)
    {
        Pool* p = m.pool_();
        return model::invoke(m, std::forward<Args>(args)...);
    }
};


}}

template <class R, class ...Args, class Pool, Pool* pool>
class function<R(Args...), Pool, pool> :
    public detail::v1::model<R(Args...), Pool, pool>
{
    using base_type = detail::v1::model<R(Args...), Pool, pool>;
    using typename base_type::model_base;

public:
    function(estd::nullptr_t) {}

    template <class F>
    function(Pool* pool2, F&& f) :
        base_type(base_type::make(pool2, std::forward<F>(f)), pool2)
    {

    }


    R operator()(Args&&...args)
    {
        return invoke(*this, std::forward<Args>(args)...);
    }
};


}}
