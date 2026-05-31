#pragma once

#include <estd/functional.h>

#include "fwd/thunk.h"
#include "internal/msg-bipbuf.h"

#if ESTD_OS_FREERTOS
#include "platform/freertos/thunk.h"
#endif

namespace embr {

inline namespace sys {

namespace detail { inline namespace v1 {

// Inspired by Boost ASIO and C# BeginInvoke
// https://www.boost.org/doc/libs/latest/doc/html/boost_asio/reference/io_context.html

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex>
class thunk : protected internal::msg_bipbuf<Buf>,
    protected Mutex
{
    using base_type = internal::msg_bipbuf<Buf>;
    using message = typename base_type::message;

    // Edge-case version which auto-invokes functor destructor immediately
    // after invocation
    using function_type = estd::detail::v2::function<
        void(void),
        estd::detail::impl::function_fnptr2_oneshot>;
    using model_base = typename function_type::model_base;

#if UNIT_TESTING
public:
#endif
    using mutex_type = Mutex;

    mutex_type& mutex() { return *this; }

    using errc = estd::errc;

public:
    thunk() = default;

    // EXPERIMENTAL
    using traits = thunk_traits<Mutex>;

    template <class ...Args>
    constexpr explicit thunk(estd::in_place_t, Args&&...args) :
        base_type{estd::in_place_t{}, std::forward<Args>(args)...}
    {}

    constexpr const mutex_type& mutex() const { return *this; }

    template <class F, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex2>
    errc post(Mutex2&& mutex, F&& f)
    {
        using model_type = function_type::model<F>;

        return base_type::template emplace<model_type>(
            std::forward<Mutex2>(mutex), std::forward<F>(f));
    }

    template <class F, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex2, class OnRetry>
    errc post_with_retry(Mutex2&& mutex, F&& f, int retry_max, OnRetry&& on_retry)
    {
        using model_type = function_type::model<F>;

        return base_type::push_with_retry(
            std::forward<Mutex2>(mutex),
            [&](message* m)
            {
                new (m->payload()) model_type(std::forward<F>(f));
            },
            sizeof(model_type),
            retry_max,
            std::forward<OnRetry>(on_retry));
    }

    template <ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex2>
    errc poll_one(Mutex2&& mutex)
    {
        return base_type::pop(std::forward<Mutex2>(mutex), [](message* m)
        {
            auto model = reinterpret_cast<model_base*>(m->payload());

            model->invoke();

            // DEBT: We'd like the option to return a success or fail code here too

        });
    }

    template <class F, class OnRetry>
    errc post_with_retry(F&& f, int retry_max, OnRetry&& on_retry)
    {
        return post_with_retry(mutex(),
            std::forward<F>(f),
            retry_max,
            std::forward<OnRetry>(on_retry));
    }

    template <class F>
    errc post(F&& f)
    {
        return post(mutex(), std::forward<F>(f));
    }

    errc poll_one()
    {
        return poll_one(mutex());
    }
};

}}  // namespace embr::inline sys::detail::inline v1

}   // namespace embr::inline sys

}   // namespace embr
