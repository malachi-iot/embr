#pragma once

#include <estd/internal/platform.h>

#include <lwip/netif.h>

namespace embr { namespace lwip {

class Netif
{
    typedef struct netif* pointer;

    pointer n;

public:
    Netif(pointer n) : n(n) {}

    bool is_null() const { return n == NULLPTR; }

    void poll()
    {
        netif_poll(n);
    }

    err_t loop_output(struct pbuf* p)
    {
        return netif_loop_output(n, p);
    }

    bool is_link_up() const
    {
        return netif_is_link_up(n);
    }

    const ip4_addr_t* ip4_addr() const
    {
        return netif_ip4_addr(n);
    }

    operator pointer() const { return n; }
};

}}