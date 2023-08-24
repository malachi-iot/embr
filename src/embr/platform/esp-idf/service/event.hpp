#pragma once

#include <esp_check.h>

#include "event.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {


template <class TSubject, class TImpl>
inline auto EventLoop::runtime<TSubject, TImpl>::on_start() -> state_result
{
    return create_start_result(esp_event_loop_create_default());
}


template <class Runtime, const esp_event_base_t& event_base_>
void EventLoop::event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    auto r = (Runtime*)arg;

    ESP_LOGV(TAG, "event_handler: %s:%" PRIi32, event_base, event_id);

    // DEBT: Consider asserting event_base == event_base_

    esp_idf::event::v1::internal::handler<event_base_>::exec(r, event_id, event_data);
}

template <const esp_event_base_t& event_base, class Runtime>
esp_err_t EventLoop::handler_register(Runtime* runtime, int32_t event_id)
{
    return esp_event_handler_register(event_base, event_id,
        EventLoop::event_handler<Runtime, event_base>, runtime);
}



template <class TSubject, class TImpl>
template <const esp_event_base_t& event_base>
esp_err_t EventLoop::runtime<TSubject, TImpl>::handler_register(
    int32_t event_id)
{
    // DEBT: Fire event that we're actually registering here
    
    return esp_event_handler_register(event_base, event_id,
        EventLoop::event_handler<runtime, event_base>, this);
}



}}

}