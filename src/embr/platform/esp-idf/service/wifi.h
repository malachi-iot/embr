#pragma once

#include <esp_wifi.h>

#include <embr/service.h>

#include "event.h"

// As per:
// https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-guides/wifi.html
// https://github.com/espressif/esp-idf/blob/v5.0/components/esp_wifi/include/esp_wifi_types.h

namespace embr::esp_idf {

namespace event { inline namespace v1 {

namespace internal {

template <>
struct handler<IP_EVENT>
{
    // DEBT: Consider passing the whole service instead of just subject -
    // doing that would mandate some fancy friend footwork
    template <class Subject>
    static void exec(Subject& subject, ip_event_t event_id, void* event_data)
    {
        // DEBT: notify this way isn't carrying forward the service as part
        // of the context

        event::v1::runtime<ip_event_t> e{event_id, event_data};
        subject.notify(e);
    }

    template <ip_event_t id>
    using ip = embr::esp_idf::event::ip<id>;

    template <class Service>
    static void exec2(Service* s, uint32_t event_id, void* event_data)
    {
        const event::v1::runtime<ip_event_t> e{(ip_event_t)event_id, event_data};
        s->notify(e);

        switch(e.id)
        {
            case IP_EVENT_STA_GOT_IP:
                s->notify(ip<IP_EVENT_STA_GOT_IP>(event_data));
                break;

            case IP_EVENT_STA_LOST_IP:
                s->notify(ip<IP_EVENT_STA_LOST_IP>(event_data));
                break;

            case IP_EVENT_ETH_GOT_IP:
                s->notify(ip<IP_EVENT_ETH_GOT_IP>(event_data));
                break;

            case IP_EVENT_ETH_LOST_IP:
                s->notify(ip<IP_EVENT_ETH_LOST_IP>(event_data));
                break;

            default: break;
        }
    }
};

}

}}

namespace service { inline namespace v1 {

struct EventService : embr::service::v1::Service
{
    typedef EventService this_type;

    static constexpr const char* TAG = "EventService";
    static constexpr const char* name() { return TAG; }

    struct event
    {
        template <typename TEventId>
        using e = embr::esp_idf::event::runtime<TEventId>;

        template <ip_event_t id>
        using ip = embr::esp_idf::event::ip<id>;
    };


    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::Service)
    
        void event_handler(ip_event_t event_id, void* event_data);

        static void ip_event_handler(void* arg, esp_event_base_t event_base,
            int32_t event_id, void* event_data);

        esp_err_t handler_register(esp_event_base_t, int32_t = ESP_EVENT_ANY_ID);
        
        template <const esp_event_base_t& event_base>
        friend struct esp_idf::event::v1::internal::handler;

        template <const esp_event_base_t&>
        static void event_handler(void* arg, esp_event_base_t event_base,
            int32_t event_id, void* event_data);

        template <const esp_event_base_t&>
        esp_err_t handler_register(int32_t = ESP_EVENT_ANY_ID);

    EMBR_SERVICE_RUNTIME_END

    // DEBT: Just a placeholder really, copy/pasted from esp_helper
    // Starts default event loop AND netif in sta mode
    esp_netif_t* create_default_sta();
};


// TODO: Make an esp event handler base class service
struct WiFi : EventService
{
    typedef WiFi this_type;

    static constexpr const char* TAG = "WiFi";
    static constexpr const char* name() { return TAG; }

    // Whether to do WiFi connects (and eventually retries, etc)
    // within this service, or to merely rebroadcast the events
    // running config/start will set this to true
    // DEBT: Use 'user' area of Service for this
    bool housekeeping_ = false;
    constexpr bool housekeeping() const { return housekeeping_; } 

    struct event : EventService::event
    {
        template <wifi_event_t id>
        using wifi = embr::esp_idf::event::wifi<id>;
    };

    EMBR_SERVICE_RUNTIME_BEGIN(EventService)

        static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

        void event_handler(int32_t event_id, void* event_data);

        void register_handler();
        esp_err_t config(wifi_mode_t mode, const wifi_init_config_t*, const wifi_config_t*);
        esp_err_t config(wifi_mode_t mode, const wifi_config_t* c)
        {
            wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();

            return config(mode, init_config, c);
        }

        state_result on_start();
        state_result on_start(wifi_mode_t mode, const wifi_config_t*);
    
    EMBR_SERVICE_RUNTIME_END
};

}}

}
