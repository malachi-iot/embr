#pragma once

#include <estd/functional.h>

#include "internal/msg-bipbuf.h"

namespace embr {

inline namespace sys {

namespace detail { inline namespace v1 {

// Inspired by Boost ASIO and C# BeginInvoke
// https://www.boost.org/doc/libs/latest/doc/html/boost_asio/reference/io_context.html

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
class thunk : protected internal::msg_bipbuf<Buf>,
    protected Mutex
{
    using base_type = internal::msg_bipbuf<Buf>;
    using mutex_type = Mutex;
    using message = typename base_type::message;

    // Edge-case version which auto-invokes functor destructor immediately
    // after invocation
    using function_type = estd::detail::v2::function<
        void(void),
        estd::detail::impl::function_fnptr2_oneshot>;
    using model_base = typename function_type::model_base;

    mutex_type& mutex() { return *this; }

public:
    thunk() = default;

    template <class ...Args>
    constexpr explicit thunk(estd::in_place_t, Args&&...args) :
        base_type{estd::in_place_t{}, std::forward<Args>(args)...}
    {}

    template <class F, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex2>
    estd::errc post(Mutex2&& mutex, F&& f)
    {
        using model_type = function_type::model<F>;

        return base_type::template emplace<model_type>(
            std::forward<Mutex2>(mutex), std::forward<F>(f));
    }

    template <ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex2>
    estd::errc poll_one(Mutex2&& mutex)
    {
        return base_type::pop(std::forward<Mutex2>(mutex), [](message* m)
        {
            auto model = reinterpret_cast<model_base*>(m->payload());

            model->invoke();

            // DEBT: We'd like the option to return a success or fail code here too

        });
    }

    template <class F>
    estd::errc post(F&& f)
    {
        return post(mutex(), std::forward<F>(f));
    }

    estd::errc poll_one()
    {
        return poll_one(mutex());
    }
};

}}  // namespace embr::inline sys::detail::inline v1
    
inline namespace v1 {

namespace layer1 {

template <unsigned sz, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
using thunk = sys::detail::v1::thunk<estd::layer1::bipbuf<sz>, Mutex>;

}

}   // namespace embr::inline sys::inline v1

}   // namespace embr::inline sys

}   // namespace embr
