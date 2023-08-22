#include "wifi_provisioner.h"

#include "event.hpp"

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

template <class Subject, class Impl>
auto WiFiProvisioner::runtime<Subject, Impl>::on_start(wifi_prov_mgr_config_t config)
    -> state_result
{
    esp_err_t ret = wifi_prov_mgr_init(config);

    EventLoop::handler_register<WIFI_PROV_EVENT>(ESP_EVENT_ANY_ID, this);

    return state_result::started();
}

}}}}
