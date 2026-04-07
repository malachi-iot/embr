#pragma once

#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// Non-owning
// Deriving from sparse_handle largely a formality - we usually treat this as a static struct,
// except in the case of sparse_function
template <class R, class ...Args, class Handle, template <class, estd::detail::impl::fn_options> class Impl>
class sparse_model<R(Args...), Handle, Impl> : public sparse_handle<
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
    explicit constexpr sparse_model(const handle_type& handle) : base_type{handle} {}

    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool, class F>
    constexpr static sparse_model make(Pool* pool, F&& f)
    {
        using model_type = typename function_type::template model<F>;

        return sparse_model(pool->template construct<model_type>(std::forward<F>(f)));
    }


    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool, class ...Args2>
    static R invoke(Pool* pool, handle_type h, Args2&&...args)
    {
        return invoke_ll(estd::is_void<R>{}, pool, h, std::forward<Args2>(args)...);
    }
};


template <class R, class ...Args, ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool,
    template <class, estd::detail::impl::fn_options> class Impl>
class sparse_function<R(Args...), Pool, Impl> : public sparse_model<R(Args...), typename Pool::handle_type>
{
    using base_type = sparse_model<R(Args...), typename Pool::handle_type>;
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
//static_assert(std::is_trivially_move_constructible<sparse_function<void(), detail::handles_traits_uint8>>::value);
#endif

}}

}}
