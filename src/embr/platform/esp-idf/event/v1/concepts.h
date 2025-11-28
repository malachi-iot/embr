#pragma once

#include <concepts>

#include <esp_event_base.h>

namespace embr::esp_idf::inline event::inline v1::inline concepts {

template <class T>
concept LoopHandle = std::same_as<T, esp_event_loop_handle_t>;

// Generally a functor, though I am suspicious if this truly works
template <typename F>
concept Functor = requires
{
    &F::operator();
};

// For enforcing non-capturing lambda or otherwise function-pointer like
// DEBT: This may fall apart when a real function pointer comes along
template <typename F>
concept EmptyFunctor = Functor<F> && std::is_empty_v<F> && requires(F f)
{
    // must convert to function pointer (which capturing lambdas can't do)
    +f;
};

}
