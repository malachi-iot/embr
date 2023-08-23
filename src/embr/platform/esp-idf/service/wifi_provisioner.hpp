#include "wifi_provisioner.h"

#include "event.hpp"
#include "internal/ble.h"

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

template <class Subject, class Impl>
esp_err_t WiFiProvisioner::runtime<Subject, Impl>::config(wifi_prov_mgr_config_t config)
{
    configuring(&config);
    
    const esp_err_t ret = wifi_prov_mgr_init(config);

    EventLoop::handler_register<WIFI_PROV_EVENT>(ESP_EVENT_ANY_ID, this);

    configured(&config);

    return ret;
}

template <class Subject, class Impl>
esp_err_t WiFiProvisioner::runtime<Subject, Impl>::config(
    wifi_prov_scheme_t scheme,
    wifi_prov_event_handler_t handler)
{
    const wifi_prov_mgr_config_t c = {
        .scheme = scheme,
        .scheme_event_handler = handler,

        // NOTE: Deprecated
        .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    return config(c);
}


template <class Subject, class Impl>
auto WiFiProvisioner::runtime<Subject, Impl>::on_start(
    wifi_prov_security_t security,
    const void *wifi_prov_sec_params,
    const char *service_name,
    const char *service_key)
    -> state_result
{
    // DEBT: current architecture is 'Starting' is ALWAYS the incoming state here -
    // so we may have to use our 'user' area or similar
    /*
    if(substate() != Configured || substate() != Paused)
    {
        ESP_LOGD(TAG, "on_start: Must be either in Configured or Paused state: %s");
        return { Error, ErrConfig };
    }   */

    esp_err_t ret = wifi_prov_mgr_start_provisioning(
        security, wifi_prov_sec_params, service_name, service_key);


    ESP_LOGD(TAG, "on_start: phase 2");

    return create_start_result(ret);
}


template <class Subject, class Impl>
void WiFiProvisioner::runtime<Subject, Impl>::pause()
{
    if(substate() != Running) return;

    // DEBT: Don't love 'pause' state here, but we aren't doing a deinit, so
    // it's not a full service stop

    state(Pausing);
    wifi_prov_mgr_stop_provisioning();
    state(Stopped, Paused);
}



}}}}
