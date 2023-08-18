#pragma once

#include <esp_check.h>

#include "wifi.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

inline esp_netif_t* EventService::create_default_sta()
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
void WiFi::runtime<TSubject, TImpl>::ip_event_handler(
    ip_event_t id, void* event_data)
{
    notify(event<ip_event_t>{id, event_data});

    switch(id)
    {
        case IP_EVENT_STA_GOT_IP:
            notify(ip_event<IP_EVENT_STA_GOT_IP>(event_data));
            break;

        case IP_EVENT_STA_LOST_IP:
            notify(ip_event<IP_EVENT_STA_LOST_IP>(event_data));
            break;

        case IP_EVENT_ETH_GOT_IP:
            notify(ip_event<IP_EVENT_ETH_GOT_IP>(event_data));
            break;

        case IP_EVENT_ETH_LOST_IP:
            notify(ip_event<IP_EVENT_ETH_LOST_IP>(event_data));
            break;

        default: break;
    }
}



template <class TSubject, class TImpl>
inline void WiFi::runtime<TSubject, TImpl>::event_handler(
    int32_t event_id, void* event_data)
{
    wifi_event_t id = (wifi_event_t)event_id;

    ESP_LOGV(TAG, "event_handler: id=%u", id);

    // Doing this explicitly rather than auto conversion back from stronger types
    // to minimize doubling up on switch statement
    notify(event<wifi_event_t>{id, event_data});

    switch(id)
    {
        case WIFI_EVENT_SCAN_DONE:
        {
            notify(wifi_event<WIFI_EVENT_SCAN_DONE>(event_data));
            break;
        }

        case WIFI_EVENT_STA_START:
        {
            notify(wifi_event<WIFI_EVENT_STA_START>(event_data));

            if(base_type::housekeeping_)
                esp_wifi_connect();

            break;
        }

        case WIFI_EVENT_STA_STOP:
        {
            notify(wifi_event<WIFI_EVENT_STA_STOP>(event_data));
            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
        {
            state(Online);
            notify(wifi_event<WIFI_EVENT_STA_CONNECTED>(event_data));
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            state(Offline);
            notify(wifi_event<WIFI_EVENT_STA_DISCONNECTED>(event_data));
            break;
        }

        case WIFI_EVENT_STA_BSS_RSSI_LOW:
        {
            // NOTE: It seems there isn't a RSSI_NORMAL event, which
            // means once we enter degraded state, we're staying there
            state(Degraded);
            notify(wifi_event<WIFI_EVENT_STA_BSS_RSSI_LOW>(event_data));
            break;
        }

        case WIFI_EVENT_WIFI_READY:
        {
            notify(wifi_event<WIFI_EVENT_WIFI_READY>(event_data));
            break;
        }

        default: break;
    }
}

template <class TSubject, class TImpl>
esp_err_t WiFi::runtime<TSubject, TImpl>::config(wifi_mode_t mode,
    const wifi_init_config_t* init_config,
    const wifi_config_t* config)
{
    esp_err_t ret = 0;

    base_type::housekeeping_ = true;

    base_type::configuring(init_config);

    ESP_GOTO_ON_ERROR(esp_wifi_init(init_config), 
        err, TAG, "initialization failed");

    ESP_GOTO_ON_ERROR(esp_wifi_set_mode(mode),
        err, TAG, "mode set failed");

    ESP_GOTO_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
        wifi_event_handler, this),
        err, TAG, "registration failed");

    // DEBT: Move IP event rebroadcaster elsewhere
    ESP_GOTO_ON_ERROR(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
        ip_event_handler, this),
        err, TAG, "registration failed");

    // DEBT: Pretty sure const_cast is safe here, but not 100% sure - verify
    // and comment
    // DEBT: mode itself could probably be compile time
    switch(mode)
    {
        case WIFI_MODE_STA:
            ESP_GOTO_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA,
                const_cast<wifi_config_t*>(config)),
                err, TAG, "configuration failed");
            break;

        case WIFI_MODE_AP:
            ESP_GOTO_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP,
                const_cast<wifi_config_t*>(config)),
                err, TAG, "configuration failed");
            break;

#ifdef CONFIG_ESP_WIFI_NAN_ENABLE
        case WIFI_MODE_NAN:
            ESP_GOTO_ON_ERROR(esp_wifi_set_config(WIFI_IF_NAN,
                const_cast<wifi_config_t*>(config)),
                err, TAG, "configuration failed");
            break;
#endif

        // DEBT: Support AP+STA mode
        // DEBT: On error, give a little more info as to why
        default:
            ret = ESP_ERR_NOT_SUPPORTED;
            goto err;
    }

    base_type::configured(init_config);
    base_type::state(Configured);
    
    return ESP_OK;

err:
    ESP_LOGW(TAG, "config failed: err=0x%04X", ret);
    state_result r = create_start_result(ret);
    base_type::state(r.state, r.substate);
    return ret;
}

template <class TSubject, class TImpl>
auto WiFi::runtime<TSubject, TImpl>::on_start() -> state_result
{
    return create_start_result(esp_wifi_start());
}


template <class TSubject, class TImpl>
auto WiFi::runtime<TSubject, TImpl>::on_start(wifi_mode_t mode,
    const wifi_config_t* c) -> state_result
{
    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();

    if(config(mode, &init_config, c) != ESP_OK)
        // DEBT: Use state as already set by config
        return state_result{Error, base_type::substate()};

    return on_start();
}



template <class TSubject, class TImpl>
void WiFi::runtime<TSubject, TImpl>::ip_event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ((runtime*)arg)->ip_event_handler((ip_event_t)event_id, event_data);
}


template <class TSubject, class TImpl>
void WiFi::runtime<TSubject, TImpl>::wifi_event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ((runtime*)arg)->event_handler(event_id, event_data);
}


template <class TSubject, class TImpl>
void WiFi::runtime<TSubject, TImpl>::register_handler()
{
    // DEBT: Make these soft errors
    // DEBT: Retain handle(s) so that we can deregister
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
        wifi_event_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
        ip_event_handler, this));
}


}}

}
