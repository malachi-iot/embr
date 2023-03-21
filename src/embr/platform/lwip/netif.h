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

    operator pointer() const { return n; }
};

}}