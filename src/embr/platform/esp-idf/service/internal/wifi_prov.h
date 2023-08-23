#pragma once

#include <wifi_provisioning/manager.h>

#include "event.h"

namespace embr { namespace esp_idf { namespace event {
    
inline namespace v1 {

template <wifi_prov_cb_event_t id>
using wifi_prov = base<wifi_prov_cb_event_t, id>;

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
    //template <ip_event_t id>
    //using ip = embr::esp_idf::event::ip<id>;

    template <class Service>
    static void exec(Service* s, uint32_t event_id, void* event_data)
    {
        const event::v1::runtime<wifi_prov_cb_event_t> e{(wifi_prov_cb_event_t)event_id, event_data};
        s->notify(e);

        switch(event_id)
        {
            case WIFI_PROV_INIT:
                s->notify(wifi_prov<WIFI_PROV_INIT>(event_data));
                break;

            case WIFI_PROV_START:
                s->notify(wifi_prov<WIFI_PROV_START>(event_data));
                break;

            case WIFI_PROV_CRED_RECV:
                s->notify(wifi_prov<WIFI_PROV_CRED_RECV>(event_data));
                break;

            case WIFI_PROV_CRED_FAIL:
                s->notify(wifi_prov<WIFI_PROV_CRED_FAIL>(event_data));
                break;

            case WIFI_PROV_CRED_SUCCESS:
                s->notify(wifi_prov<WIFI_PROV_CRED_SUCCESS>(event_data));
                break;

            case WIFI_PROV_END:
                s->notify(wifi_prov<WIFI_PROV_END>(event_data));
                break;

            case WIFI_PROV_DEINIT:
                break;
        }
    }
};

}

}

}}}
