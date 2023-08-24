#pragma once

#include <esp_wifi.h>

#include <embr/service.h>

#include "event.h"

// As per:
// https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-guides/wifi.html
// https://github.com/espressif/esp-idf/blob/v5.0/components/esp_wifi/include/esp_wifi_types.h

namespace embr::esp_idf {

namespace service { inline namespace v1 {

struct NetIf : embr::service::v1::SparseService
{
    using this_type = NetIf;
    
    static constexpr const char* TAG = "NetIf";
    static constexpr const char* name() { return TAG; }

    state_result on_start() const
    {
        return create_start_result(esp_netif_init());
    }

    EMBR_SERVICE_RUNTIME_BEGIN(SparseService)

    EMBR_SERVICE_RUNTIME_END
};

// TODO: Make an esp event handler base class service
struct WiFi : embr::service::v1::Service
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

    struct event
    {
        template <ip_event_t id>
        using ip = embr::esp_idf::event::ip<id>;

        template <wifi_event_t id>
        using wifi = embr::esp_idf::event::wifi<id>;
    };

    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::Service)

        static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

        void event_handler(int32_t event_id, void* event_data);

        void register_handler();

        // minimal init bringup, mainly for use with provisioning
        esp_err_t config();

        esp_err_t config(wifi_mode_t mode, const wifi_init_config_t*, const wifi_config_t*);
        esp_err_t config(wifi_mode_t mode, const wifi_config_t* c)
        {
            wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();

            return config(mode, init_config, c);
        }

        state_result on_start();
        state_result on_start(wifi_mode_t mode, const wifi_config_t*);

        // Called when lower level depended-on services start up
        template <class Subject2, class Impl2>
        bool config_event_loop(EventLoop::runtime<Subject2, Impl2>&);
        bool config_netif();

        template <class Subject2, class Impl2>
        void on_notify(changed<id::state>, EventLoop::runtime<Subject2, Impl2>&);
        void on_notify(changed<id::state>, const Flash&);
        void on_notify(changed<id::state>, const NetIf&);
    
    EMBR_SERVICE_RUNTIME_END

    // DEBT: Copy/pasting this everywhere sucks
    template <class TSubject>
    using static_type = static_factory<TSubject, this_type>::static_type;
};

}}

}
