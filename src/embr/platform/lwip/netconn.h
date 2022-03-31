/**
 * 
 *  References:
 * 
 * 1. https://lwip.fandom.com/wiki/Raw/UDP
 * 2. https://www.nongnu.org/lwip/2_0_x/group__udp__raw.html
 * 3. https://www.nongnu.org/lwip/2_0_x/group__netconn.html
 *
 */
#pragma once

extern "C" {

#include <lwip/api.h>

}

#include "netbuf.h"
#include <estd/internal/platform.h>

namespace embr { namespace lwip {

class Netconn
{
    typedef struct netconn value_type;
    typedef value_type* pointer;
    typedef struct netbuf netbuf_type;

    pointer conn;

public:
    bool new_with_proto_and_callback(netconn_type t, uint8_t proto, netconn_callback callback)
    {
        conn = netconn_new_with_proto_and_callback(t, proto, callback);
        return conn != NULLPTR;
    }

    err_t del()
    {
        return netconn_delete(conn);
    }

    err_t bind(const ip_addr_t* addr, uint16_t port)
    {
        return netconn_bind(conn, addr, port);
    }

    err_t connect(const ip_addr_t* addr, uint16_t port)
    {
        return netconn_connect(conn, addr, port);
    }

    /**
     * @brief "Only valid for UDP netconns" [3]
     * 
     * @return err_t 
     */
    err_t disconnect()
    {
        return netconn_disconnect(conn);
    }

    /**
     * @brief "Send data over a UDP or RAW netconn (that is already connected)." [3]
     * 
     * @param buf 
     * @return err_t 
     */
    err_t send(netbuf_type* buf)
    {
        return netconn_send(conn, buf);
    }

    /**
     * @brief "Send data (in form of a netbuf) to a specific remote IP address and port. 
     *         Only to be used for UDP and RAW netconns (not TCP)." [3]
     * 
     * @param buf 
     * @param addr 
     * @param port 
     * @return err_t 
     */
    err_t send(netbuf_type* buf, const ip_addr_t* addr, uint16_t port)
    {
        return netconn_sendto(conn, buf, addr, port);
    }
};

}}