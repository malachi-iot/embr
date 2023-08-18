#pragma once

#include <esp_wifi.h>

#include "event.h"

namespace embr { namespace esp_idf { namespace event {

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




template <ip_event_t id>
using ip = base<ip_event_t, id>;


}}}