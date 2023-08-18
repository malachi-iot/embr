#pragma once

#include <esp_wifi.h>

#include "event.h"

namespace embr { namespace esp_idf { namespace event { inline namespace v1 {

template <ip_event_t id>
using ip = base<ip_event_t, id>;

namespace internal {

template <>
struct mapping<ip_event_t, IP_EVENT_STA_GOT_IP>
{
    typedef ip_event_got_ip_t type;
};



template <>
struct mapping<ip_event_t, IP_EVENT_ETH_GOT_IP>
{
    typedef ip_event_got_ip_t type;
};


template <>
struct handler<IP_EVENT>
{
    template <ip_event_t id>
    using ip = embr::esp_idf::event::ip<id>;

    template <class Service>
    static void exec(Service* s, uint32_t event_id, void* event_data)
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


}}}}