#pragma once

#include <embr/service.h>

#include <wifi_provisioning/manager.h>

#include "internal/wifi_prov.h"

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

struct WiFiProvisioner : embr::service::v1::Service
{
    typedef WiFiProvisioner this_type;

    static constexpr const char* TAG = "WiFi Provisioner";
    static constexpr const char* name() { return TAG; }

    state_result on_stop()
    {
        wifi_prov_mgr_deinit();
        return { Stopped, Finished };
    }

    constexpr unsigned retry_max() const { return state_.user2.v2; }

    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::Service)

    // DEBT: This is just awkward
    template <const esp_event_base_t& event_base>
    friend struct esp_idf::event::v1::internal::handler;

    static void event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);

    esp_err_t config(wifi_prov_mgr_config_t);
    esp_err_t config(wifi_prov_scheme_t,
        wifi_prov_event_handler_t = WIFI_PROV_EVENT_HANDLER_NONE);

    state_result on_start(wifi_prov_security_t,
        const void* wifi_prov_sec_params,
        const char* service_name,
        const char* service_key = nullptr);
    void pause();

    EMBR_SERVICE_RUNTIME_END
};

}}}}