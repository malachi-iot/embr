#pragma once

#include "udp.h"

namespace embr { namespace lwip { namespace experimental {


struct TransportBase
{
    typedef const ip_addr_t* addr_pointer;

    struct Endpoint
    {
        addr_pointer address;
        uint16_t port;
    };

    typedef Endpoint endpoint_type;

};

struct TransportUdp : TransportBase
{
    void send() {}
};

}}}