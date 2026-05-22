#pragma once

#include <estd/functional.h>

#include "internal/msg-bipbuf.h"

namespace embr {

inline namespace sys {

namespace detail { inline namespace v1 {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf>
class thunk : protected internal::msg_bipbuf<Buf>
{
    using base_type = internal::msg_bipbuf<Buf>;
    using message = typename base_type::message;

    // Edge-case version which auto-invokes functor destructor immediately
    // after invocation
    using function_type = estd::detail::v2::function<
        void(void),
        estd::detail::impl::function_fnptr2_oneshot>;
    using model_base = typename function_type::model_base;

public:
    template <class F, class Mutex2>
    estd::errc enqueue(F&& f, Mutex2&& mutex)
    {
        using model_type = function_type::model<F>;

        return base_type::template emplace<model_type>(
            std::forward<Mutex2>(mutex), std::forward<F>(f));
    }

    template <class Mutex2>
    estd::errc poll_one(Mutex2&& mutex)
    {
        // DEBT: mutex should be as first parameter on pop to match emplace
        return base_type::pop([&](message* m)
        {
            auto model = reinterpret_cast<model_base*>(m->payload());

            model->invoke();

            // DEBT: We'd like the option to return a success or fail code here too

        }, std::forward<Mutex2>(mutex));
    }
};

}}  // namespace embr::inline sys::detail::inline v1
    
inline namespace v1 {

}   // namespace embr::inline sys::inline v1

}   // namespace embr::inline sys

}   // namespace embr
