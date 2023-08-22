#pragma once

#include <wifi_provisioning/manager.h>

#include "event.h"

namespace embr { namespace esp_idf { namespace event {
    
inline namespace v1 {

namespace internal {

template <>
struct mapping<wifi_prov_cb_event_t, WIFI_PROV_CRED_RECV>
    : estd::type_identity<wifi_sta_config_t> {};

template <>
struct mapping<wifi_prov_cb_event_t, WIFI_PROV_CRED_FAIL>
    : estd::type_identity<wifi_prov_sta_fail_reason_t> {};

template <>
struct handler<WIFI_PROV_EVENT>
{
    template <class Service>
    static void exec(Service* s, uint32_t event_id, void* event_data)
    {
        const event::v1::runtime<wifi_prov_cb_event_t> e{(wifi_prov_cb_event_t)event_id, event_data};
        s->notify(e);

        /*
        switch(event_id)
        {
            case WIFI_PROV_CRED_RECV
        }   */
    }
};

}

}

}}}
