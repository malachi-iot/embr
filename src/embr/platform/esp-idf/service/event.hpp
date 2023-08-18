#pragma once

#include <esp_check.h>

#include "event.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

inline esp_netif_t* Event::create_default_sta()
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


template <class TSubject, class TImpl>
void Event::runtime<TSubject, TImpl>::event_handler(
    ip_event_t id, void* event_data)
{
    notify(event::e<ip_event_t>{id, event_data});

    switch(id)
    {
        case IP_EVENT_STA_GOT_IP:
            notify(event::ip<IP_EVENT_STA_GOT_IP>(event_data));
            break;

        case IP_EVENT_STA_LOST_IP:
            notify(event::ip<IP_EVENT_STA_LOST_IP>(event_data));
            break;

        case IP_EVENT_ETH_GOT_IP:
            notify(event::ip<IP_EVENT_ETH_GOT_IP>(event_data));
            break;

        case IP_EVENT_ETH_LOST_IP:
            notify(event::ip<IP_EVENT_ETH_LOST_IP>(event_data));
            break;

        default: break;
    }
}


template <class TSubject, class TImpl>
void Event::runtime<TSubject, TImpl>::ip_event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ((runtime*)arg)->event_handler((ip_event_t)event_id, event_data);
}




// DEBT: Consider compile time specialization for optimization, compile time
// indication of support, and extensibility
template <class TSubject, class TImpl>
esp_err_t Event::runtime<TSubject, TImpl>::handler_register(
    esp_event_base_t event_base, int32_t event_id)
{
    if(event_base == IP_EVENT)
        return esp_event_handler_register(IP_EVENT, event_id, ip_event_handler, this);

    return ESP_ERR_NOT_FOUND;
}

template <class TSubject, class TImpl>
template <const esp_event_base_t& event_base_>
void Event::runtime<TSubject, TImpl>::event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    auto r = (runtime*)arg;

    // DEBT: Consider asserting event_base == event_base_

    esp_idf::event::v1::internal::handler<event_base_>::exec2(r, event_id, event_data);
}


template <class TSubject, class TImpl>
template <const esp_event_base_t& event_base>
esp_err_t Event::runtime<TSubject, TImpl>::handler_register(
    int32_t event_id)
{
    return esp_event_handler_register(event_base, event_id,
        event_handler<event_base>, this);
}



}}

}