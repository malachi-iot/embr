#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/netbuf.h>

}

#include <estd/internal/platform.h>

namespace embr { namespace lwip {

class Netconn
{
    typedef struct netconn value_type;
    typedef value_type* pointer;

    pointer conn;

public:
    bool new_with_proto_and_callback(netconn_type t, uint8_t proto, netconn_callback callback)
    {
        conn = netconn_new_with_proto_and_callback(t, proto, callback);
        return conn != NULLPTR;
    }

    err_t bind(const ip_addr_t* addr, uint16_t port)
    {
        return netconn_bind(conn, addr, port);
    }
};

}}