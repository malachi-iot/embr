#pragma once

#include <esp_event.h>

template <esp_event_base_t EventBase, int32_t event_id>
struct embr_idf_event_traits
{
    static constexpr bool specialized = false;
    static constexpr int32_t id = event_id;
    static constexpr const char* id_name = "unspecified";
    static constexpr const char* event_base = EventBase;
    using type = void;
};


#define EMBR_IDF_EVENT_TRAITS(_enum, event_id, payload) \
template <> \
struct embr_idf_event_traits<_enum, event_id> \
{ \
    static constexpr bool specialized = true; \
    static constexpr int32_t id = event_id; \
    static constexpr const char* id_name = #event_id; \
    static constexpr const char* event_base = _enum; \
    using type = payload; \
};


namespace embr::esp_idf::inline event {

// EXPERIMENTAL
esp_err_t handler_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void* event_handler_arg = nullptr)
{
    return esp_event_handler_register(event_base, event_id, event_handler, event_handler_arg);
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
esp_err_t handler_register_with(esp_event_loop_handle_t event_loop, esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void* event_handler_arg)
{
    return esp_event_handler_register_with(event_loop, event_base, event_id, event_handler, event_handler_arg);
}

template <class Data>
esp_err_t post(esp_event_base_t event_base, int32_t event_id, Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post(event_base, event_id, event_data, sizeof(Data),
        ticks_to_wait);
}


esp_err_t post(esp_event_base_t event_base, int32_t event_id,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post(event_base, event_id, nullptr, 0, ticks_to_wait);
}

template <class Data>
esp_err_t post(esp_event_loop_handle_t el, esp_event_base_t event_base,
    int32_t event_id, Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post_to(el, event_base, event_id,
        event_data, sizeof(Data), ticks_to_wait);
}

esp_err_t post(esp_event_loop_handle_t el, esp_event_base_t event_base,
    int32_t event_id, TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post_to(el, event_base, event_id,
        nullptr, 0, ticks_to_wait);
}

}

