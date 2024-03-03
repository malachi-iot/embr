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
protected:
    typedef struct netconn value_type;
    typedef value_type* pointer;
    typedef struct netbuf netbuf_type;

    pointer conn;

public:
    Netconn() = default;
    Netconn(pointer conn) : conn{conn} {}

    bool has_conn() const { return conn != NULLPTR; }

    bool new_with_proto_and_callback(netconn_type t, uint8_t proto, netconn_callback callback)
    {
        conn = netconn_new_with_proto_and_callback(t, proto, callback);
        return has_conn();
    }

    bool alloc(netconn_type t, uint8_t proto, netconn_callback callback = nullptr)
    {
        conn = netconn_new_with_proto_and_callback(t, proto, callback);
        return has_conn();
    }

    bool alloc(netconn_type t, netconn_callback callback = nullptr)
    {
        conn = netconn_new_with_proto_and_callback(t, 0, callback);
        return has_conn();
    }

    err_t del()
    {
        return netconn_delete(conn);
    }

    /**
     * @brief "Accept a new connection on a TCP listening netconn." [3]
     * 
     * @param new_conn "pointer where the new connection is stored" [3]
     * @return err_t 
     */
    err_t accept(value_type** new_conn)
    {
        return netconn_accept(conn, new_conn);
    }

    err_t accept(Netconn* new_conn)
    {
        return netconn_accept(conn, &new_conn->conn);
    }

    err_t bind(uint16_t port)
    {
        return netconn_bind(conn, NULLPTR, port);
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

    err_t getaddr(ip_addr_t* addr, uint16_t* port, bool local)
    {
        return netconn_getaddr(conn, addr, port, local);
    }

    err_t listen()
    {
        return netconn_listen(conn);
    }

    err_t recv(netbuf_type** buf)
    {
        return netconn_recv(conn, buf);
    }

    err_t recv(Netbuf* buf)
    {
        return netconn_recv(conn, &buf->buf);
    }

    err_t recv_tcp_pbuf(pbuf** new_pbuf, uint8_t flags)
    {
        return netconn_recv_tcp_pbuf_flags(conn, new_pbuf, flags);
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

    template <bool use_pointer>
    err_t send(netbuf_type* buf, internal::Endpoint<use_pointer> e)
    {
        return netconn_sendto(conn, buf, e.address(), e.port());
    }

    /**
     * @brief "Shut down one or both sides of a TCP netconn (doesn't delete it)" [3]
     * 
     * @param shut_rx 
     * @param shut_tx 
     * @return err_t 
     */
    err_t shutdown(bool shut_rx, bool shut_tx)
    {
        return netconn_shutdown(conn, shut_rx, shut_tx);
    }

    bool nonblocking() const
    {
        return netconn_is_nonblocking(conn);
    }

    void nonblocking(bool val)
    {
        netconn_set_nonblocking(conn, val);
    }

    err_t write_partly(const void* dataptr, size_t size, uint8_t flags, size_t* bytes_written)
    {
        return netconn_write_partly(conn, dataptr, size, flags, bytes_written);
    }

    // EXPERIMENTAL
    constexpr pointer native() const { return conn; }
};

namespace experimental {

class AutoNetconn : public Netconn
{
    err_t err;

public:
    AutoNetconn(netconn_type t, netconn_callback callback = NULLPTR)
    {
        new_with_proto_and_callback(t, 0, callback);
    }

    AutoNetconn(netconn_type t, uint8_t proto, netconn_callback callback = NULLPTR)
    {
        new_with_proto_and_callback(t, proto, callback);
    }

    ~AutoNetconn()
    {
        if(has_conn())
            del();
    }


    err_t bind(uint16_t port)
    {
        return err = netconn_bind(conn, NULLPTR, port);
    }

    err_t last_err() const { return err; }


    AutoNetbuf recv()
    {
        netbuf_type* buf;
        err = netconn_recv(conn, &buf);
        return AutoNetbuf(buf);
    }

};

}


}}
