#pragma once

#include <esp_event.h>

template <esp_event_base_t event_base, int32_t event_id>
struct embr_idf_event_traits
{
    static constexpr bool specialized = false;
    static constexpr bool is_property = false;
    static constexpr int32_t id = event_id;
    static constexpr const char* id_name = "unspecified";
    static constexpr const char* base = event_base;
    using type = void;
    static constexpr const char* type_name = id_name;
};


#define EMBR_IDF_EVENT_TRAITS(event_base, event_id, payload) \
template <> \
struct embr_idf_event_traits<event_base, event_id> \
{ \
    static constexpr bool specialized = true; \
    static constexpr bool is_property = false; \
    static constexpr int32_t id = event_id; \
    static constexpr const char* id_name = #event_id; \
    static constexpr const char* base = event_base; \
    using type = payload; \
    static constexpr const char* type_name = #payload; \
};

// DEBT: Inconsistent naming with above
#define EMBR_ESP_EVENT_DECLARE_BASE(id) constexpr const char id[] = #id

namespace embr::esp_idf::inline event {

template <esp_event_base_t event_base, int32_t event_id>
using event_traits = embr_idf_event_traits<event_base, event_id>;

// EXPERIMENTAL
esp_err_t handler_register(esp_event_base_t event_base, int32_t event_id,
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
esp_err_t handler_register(esp_event_loop_handle_t event_loop, esp_event_base_t event_base, int32_t event_id,
    esp_event_handler_t event_handler, void* event_handler_arg = nullptr)
{
    return esp_event_handler_instance_register_with(
        event_loop, event_base, event_id, event_handler, event_handler_arg, nullptr);
}

// EXPERIMENTAL
template <esp_event_base_t event_base, int32_t event_id,
    class Data = typename event_traits<event_base, event_id>::type, class F>
esp_err_t handler_register_exp(F& f)
{
    return esp_event_handler_instance_register(event_base, event_id,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
        {
            static_cast<F*>(arg)->operator()(static_cast<Data*>(event_data));
        }, &f, nullptr);
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

