#pragma once

#include <esp_event.h>

#include "fwd.h"
#include "traits.h"

namespace embr::esp_idf::inline event::inline v1 {

// EXPERIMENTAL
inline esp_err_t handler_register(esp_event_base_t event_base, int32_t event_id,
    esp_event_handler_t event_handler, void* event_handler_arg = nullptr,
    esp_event_handler_instance_t* instance = nullptr)
{
    return esp_event_handler_instance_register(
        event_base, event_id, event_handler, event_handler_arg, instance);
}

// EXPERIMENTAL
template <class F>
esp_err_t handler_register_exp(esp_event_base_t event_base, int32_t event_id, F& f)
{
    return esp_event_handler_register(event_base, event_id,
        [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
        {
            static_cast<F*>(arg)->operator()(event_base, event_id, event_data);
        }, &f);
}

// EXPERIMENTAL
inline esp_err_t handler_register(esp_event_loop_handle_t event_loop, esp_event_base_t event_base, int32_t event_id,
    esp_event_handler_t event_handler, void* event_handler_arg = nullptr)
{
    return esp_event_handler_instance_register_with(
        event_loop, event_base, event_id, event_handler, event_handler_arg, nullptr);
}

// EXPERIMENTAL
template <esp_event_base_t event_base, int32_t event_id,
    class Data = typename event_traits_legacy<event_base, event_id>::type, class F>
esp_err_t handler_register_exp(F& f)
{
    return esp_event_handler_instance_register(event_base, event_id,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
        {
            static_cast<F*>(arg)->operator()(static_cast<Data*>(event_data));
        }, &f, nullptr);
}


template <auto event_id,
    class Data = typename event_traits<event_id>::data_type, class F>
esp_err_t handler_register(F& f,
    esp_event_handler_instance_t* instance = nullptr)
{
    using traits = event_traits<event_id>;

    return esp_event_handler_instance_register(traits::base(), event_id,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
        {
            static_cast<F*>(arg)->operator()(static_cast<Data*>(event_data));
        }, &f, instance);
}


template <auto event_id>
esp_err_t handler_register(esp_event_loop_handle_t loop,
    esp_event_handler_t event_handler,
    void* event_handler_arg = nullptr,
    esp_event_handler_instance_t* instance = nullptr)
{
    using traits = event_traits<event_id>;

    return esp_event_handler_instance_register_with(
        loop, traits::base(), event_id, event_handler, event_handler_arg, instance);
}

// TODO: Disambiguate with is_invocable_v so that above can coeexist
template <auto event_id,
    class Data = typename event_traits<event_id>::data_type,
    class Arg = void, class F>
esp_err_t handler_register_exp(esp_event_loop_handle_t loop, F&& f,
    Arg* event_handler_arg,
    esp_event_handler_instance_t* instance = nullptr)
{
    using traits = event_traits<event_id>;

    // DEBT: Crude and possibly UB
    static_assert(sizeof(F) <= 1, "Only non-capturing lambdas are supported");

    return esp_event_handler_instance_register_with(loop, traits::base(), event_id,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
        {
            F{}(static_cast<Arg*>(arg), static_cast<Data*>(event_data));
        }, event_handler_arg, instance);
}


// TODO: Disambiguate with is_invocable_v so that above can coeexist
template <auto event_id,
    class Data = typename event_traits<event_id>::data_type,
    class F>
esp_err_t handler_register_exp(esp_event_loop_handle_t loop, F&& f,
    esp_event_handler_instance_t* instance = nullptr)
{
    using traits = event_traits<event_id>;

    // DEBT: Crude and possibly UB
    static_assert(sizeof(F) <= 1, "Only non-capturing lambdas are supported");

    return esp_event_handler_instance_register_with(loop, traits::base(), event_id,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
        {
            F{}(static_cast<Data*>(event_data));
        }, nullptr, instance);
}


template <auto event_id,
    class Data = typename event_traits<event_id>::data_type, class F>
esp_err_t handler_register(esp_event_loop_handle_t loop, F& f,
    esp_event_handler_instance_t* instance = nullptr)
{
    using traits = event_traits<event_id>;

    return esp_event_handler_instance_register_with(loop, traits::base(), event_id,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
        {
            static_cast<F*>(arg)->operator()(static_cast<Data*>(event_data));
        }, &f, instance);
}



template <auto event_id, class Traits = event_traits<event_id>>
esp_err_t post(
    const typename Traits::data_type* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    static_assert(Traits::is_specialized, "event_traits must be specialized");

    return esp_event_post(Traits::base(), event_id,
        event_data, sizeof(typename Traits::data_type),
        ticks_to_wait);
}


template <auto event_id, class Traits = event_traits<event_id>>
esp_err_t post(esp_event_loop_handle_t loop_handle,
    const typename Traits::data_type* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    static_assert(Traits::is_specialized, "event_traits must be specialized");

    return esp_event_post_to(loop_handle, Traits::base(), event_id,
        event_data, sizeof(typename Traits::data_type),
        ticks_to_wait);
}

template <esp_event_base_t event_base, int32_t event_id, class Traits = event_traits_legacy<event_base, event_id>>
esp_err_t post(typename Traits::type* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    static_assert(Traits::is_specialized, "event_traits must be specialized");

    return esp_event_post(event_base, event_id, event_data, sizeof(typename Traits::type),
        ticks_to_wait);
}

#if FEATURE_EMBR_ESP_EVENT_UNSAFE
template <class Data>
esp_err_t post(esp_event_base_t event_base, int32_t event_id, Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post(event_base, event_id, event_data, sizeof(Data),
        ticks_to_wait);
}
#endif


inline esp_err_t post(esp_event_base_t event_base, int32_t event_id,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post(event_base, event_id, nullptr, 0, ticks_to_wait);
}


// FIX: This isn't working right.  If we try to use him from state_base.set, event_data gets corrupted
template <esp_event_base_t event_base, int32_t event_id, class Traits = event_traits_legacy<event_base, event_id>>
esp_err_t post(esp_event_loop_handle_t el,
    typename Traits::type* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    static_assert(Traits::is_specialized, "event_traits must be specialized");

    return esp_event_post_to(el, event_base, event_id,
        event_data, sizeof(sizeof(typename Traits::type)), ticks_to_wait);
}

#if FEATURE_EMBR_ESP_EVENT_UNSAFE
template <class Data>
esp_err_t post(esp_event_loop_handle_t el, esp_event_base_t event_base,
    int32_t event_id, Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post_to(el, event_base, event_id,
        event_data, sizeof(Data), ticks_to_wait);
}
#endif

inline esp_err_t post(esp_event_loop_handle_t el, esp_event_base_t event_base,
    int32_t event_id, TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post_to(el, event_base, event_id,
        nullptr, 0, ticks_to_wait);
}



}

