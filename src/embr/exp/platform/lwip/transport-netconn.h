#pragma once

#include <estd/span.h>

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/endpoint.h"
#include "embr/platform/lwip/streambuf.h"

// Has some issues and don't want to spend time fixing them on an experimental branch -
// we can fall back to non-wrapped true LwIP API
//#include "embr/platform/lwip/netconn.h"

#include "core.h"
#include "pbuf.h"
#include "netbuf.h"

namespace embr { namespace experimental {

// TODO: Add consideration for LWIP_SO_RCVTIMEO as mentioned:
// https://stackoverflow.com/questions/10848851/lwip-rtos-how-to-avoid-netconn-block-the-thread-forever
// https://lwip.fandom.com/wiki/Netconn_receive_timeout 
// https://www.nongnu.org/lwip/2_0_x/group__lwip__opts__socket.html#ga91af3ade95b20b9a60c65ed0380fa0ed
// appears to be always on for esp-idf, conveniently


template <>
struct protocol_traits<netconn>
{
    typedef netconn_type type;
};


struct netconn_transport_traits_base :
    tags::connection
{
protected:
    typedef struct netconn native_type;

public:
    typedef native_type* transport_type;
    typedef transport_result_wrapper<err_t> result_type;
    typedef lwip::endpoint endpoint_type;

    static result_type connect(transport_type conn, const endpoint_type& endpoint)
    {
        return netconn_connect(conn, endpoint.addr, endpoint.port);
    }

    static result_type free(transport_type conn)
    {
        return netconn_delete(conn);
    }
};



template <>
struct transport_traits2<netconn, NETCONN_TCP> :
    netconn_transport_traits_base,
    tags::write_polling,
    tags::stream
{
public:
    typedef netbuf* ibuf_type;
    typedef estd::span<const void*> obuf_type;

    static transport_type create()
    {
        return netconn_new(NETCONN_TCP);
    }

/*
    enum class protocol_type
    {
        tcp_ipv4,
        tcp_ipv6,
        udp_ipv4,
        udp_ipv6
    }; */
    //typedef netconn_type protocol_type;


    struct transaction
    {
        transport_type n;
        obuf_type buf;
        size_t bytes_written;
    };

    // System allocates netbuf here
    static void read(transport_type n, ibuf_type* new_buf)
    {

    }

    static void begin_write(transport_type n, transaction* t)
    {
        t->n = n;
    }


    static result_type write(transaction* t)
    {
        // tcp only
        return netconn_write_partly(t->n, t->buf.data(), t->buf.size(), NETCONN_DONTBLOCK, &t->bytes_written);
    }


    static void end_write(transaction* t)
    {

    }

    static constexpr int got_here() { return 1; }

    static result_type disconnect(transport_type conn)
    {
        // shuts down both read and write parts of the connection
        return netconn_shutdown(conn, 1, 1);
    }
};


template <>
struct transport_traits2<netconn, NETCONN_UDP> :
    netconn_transport_traits_base,
    tags::write_polling,
    tags::datagram
{
    typedef native_type* transport_type;
    typedef netbuf* ibuf_type;
    typedef netbuf* obuf_type;

    static transport_type create()
    {
        return netconn_new(NETCONN_UDP);
    }

    static result_type read(transport_type n, ibuf_type* in)
    {
        return netconn_recv(n, in);
    }

    static result_type write(transport_type n, obuf_type buf)
    {
        // udp or raw only
        return netconn_send(n, buf);
    }

    static result_type write(transport_type n, obuf_type buf, const lwip::endpoint& endpoint)
    {
        // udp or raw only
        return netconn_sendto(n, buf, endpoint.addr, endpoint.port);
    }

    static result_type disconnect(transport_type conn)
    {
        return netconn_disconnect(conn);
    }
};

/*
template <>
struct transport_traits_wrapper<transport_traits<netconn>, transport_traits<netconn>::protocol_type::tcp_ipv4>
{
    static constexpr int got_here() { return 1; }
}; */

/*
template <>
struct transport_traits_wrapper<transport_traits<netconn>, NETCONN_TCP>
{
    static constexpr int got_here() { return 1; }
};



template <class TTransport, typename TTransport::protocol_type p>
void auxliary_usage_test(transport_traits_wrapper<TTransport, p>& v)
{

}
*/

template <class TNativeTransport, typename protocol_traits<TNativeTransport>::type p>
void auxliary_usage_test2(transport_traits2<TNativeTransport, p>& v)
{

}


}}