#pragma once

#include <esp_event.h>

namespace embr::esp_idf::inline event {

// EXPERIMENTAL
esp_err_t handler_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void* event_handler_arg = nullptr)
{
    return esp_event_handler_register(event_base, event_id, event_handler, event_handler_arg);
}

// EXPERIMENTAL
esp_err_t handler_register_with(esp_event_loop_handle_t event_loop, esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void* event_handler_arg)
{
    return esp_event_handler_register_with(event_loop, event_base, event_id, event_handler, event_handler_arg);
}

template <class Data>
esp_err_t post(
    esp_event_base_t event_base,
    int32_t event_id,
    Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post(
        event_base,
        event_id,
        event_data,
        sizeof(Data),
        ticks_to_wait);
}


template <class Data>
esp_err_t post(
    esp_event_loop_handle_t el,
    esp_event_base_t event_base,
    int32_t event_id,
    Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post_to(
        el,
        event_base,
        event_id,
        event_data,
        sizeof(Data),
        ticks_to_wait);
}

}

