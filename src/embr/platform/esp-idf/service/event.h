#pragma once

#include <esp_wifi.h>

#include <embr/service.h>

#include "internal/ip.h"
#include "result.h"
#include "internal/wifi.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

struct Event : embr::service::v1::Service
{
    typedef Event this_type;

    static constexpr const char* TAG = "Event";
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


}}

}
