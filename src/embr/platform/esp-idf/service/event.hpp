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


inline esp_netif_t* EventLoop::create_default_sta()
{
    ESP_LOGD(TAG, "create default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // NOTE: Examples always put esp_netif_init ahead of event loop, but in my mind
    // these are separate concerns with esp_netif_init relying on event loop, but not vice versa.
    // Therefore, I place esp_event_loop_create_default() first
    // "This function should be called exactly once from application code, when the application starts up."
    // https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/network/esp_netif.html
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_t* wifi_netif = esp_netif_create_default_wifi_sta();
    return wifi_netif;
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


template <class TSubject, class TImpl>
template <const esp_event_base_t& event_base>
esp_err_t EventLoop::runtime<TSubject, TImpl>::handler_register(
    int32_t event_id)
{
    return esp_event_handler_register(event_base, event_id,
        EventLoop::event_handler<runtime, event_base>, this);
}



}}

}