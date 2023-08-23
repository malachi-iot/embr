#pragma once

#include "event.h"

namespace embr { namespace esp_idf { namespace event { inline namespace v1 {

template <protocomm_transport_ble_event_t id>
using protocomm_ble = base<protocomm_transport_ble_event_t, id>;

namespace internal {

template <>
struct handler<PROTOCOMM_TRANSPORT_BLE_EVENT>
{
    template <class Service>
    static void exec(Service* s, uint32_t event_id, void* event_data)
    {
        const event::v1::runtime<protocomm_transport_ble_event_t> e{(protocomm_transport_ble_event_t)event_id, event_data};
        s->notify(e);

        switch(event_id)
        {
            case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
                s->notify(protocomm_ble<PROTOCOMM_TRANSPORT_BLE_CONNECTED>(event_data));
                break;

            case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
                s->notify(protocomm_ble<PROTOCOMM_TRANSPORT_BLE_DISCONNECTED>(event_data));
                break;
        }
    } 
};

}

}}}}