#pragma once

#include <embr/service.h>

#include <wifi_provisioning/manager.h>

#include "internal/wifi_prov.h"

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

struct WiFiProvisioner : embr::service::v1::Service
{
    typedef WiFiProvisioner this_type;

    state_result on_stop()
    {
        wifi_prov_mgr_deinit();
        return { Stopped, Finished };
    }

    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::Service)

    // DEBT: This is just awkward
    friend class esp_idf::event::v1::internal::handler<WIFI_PROV_EVENT>;

    state_result on_start(wifi_prov_mgr_config_t);

    EMBR_SERVICE_RUNTIME_END
};

}}}}