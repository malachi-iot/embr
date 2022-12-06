#pragma once

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


template <>
struct transport_traits<netconn> :
    tags::write_polling
{
private:
    typedef struct netconn native_type;

public:
    typedef native_type* transport_type;
    typedef netbuf* ibuf_type;
    typedef netbuf* obuf_type;
    typedef transport_result_wrapper<err_t> result_type;

/*
    enum class protocol_type
    {
        tcp_ipv4,
        tcp_ipv6,
        udp_ipv4,
        udp_ipv6
    }; */
    typedef netconn_type protocol_type;


    struct transaction
    {
        transport_type n;
        const void* dataptr;
        size_t size;
        size_t bytes_written;
    };

    // System allocates netbuf here
    static void read(transport_type n, ibuf_type* new_buf)
    {

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

    static void begin_write(transport_type n, transaction* t)
    {
        t->n = n;
    }


    static result_type write(transaction* t)
    {
        // tcp only
        return netconn_write_partly(t->n, t->dataptr, t->size, NETCONN_DONTBLOCK, &t->bytes_written);
    }


    static void end_write(transaction* t)
    {

    }
};


/*
template <>
struct transport_traits_wrapper<transport_traits<netconn>, transport_traits<netconn>::protocol_type::tcp_ipv4>
{
    static constexpr int got_here() { return 1; }
}; */

template <>
struct protocol_traits<netconn>
{
    typedef netconn_type type;
};


template <>
struct transport_traits_wrapper<transport_traits<netconn>, NETCONN_TCP>
{
    static constexpr int got_here() { return 1; }
};

template <>
struct transport_traits_wrapper2<netconn, NETCONN_TCP>
{
    static constexpr int got_here() { return 1; }
};


template <class TTransport, typename TTransport::protocol_type p>
void auxliary_usage_test(transport_traits_wrapper<TTransport, p>& v)
{

}

template <class TNativeTransport, typename protocol_traits<TNativeTransport>::type p>
void auxliary_usage_test2(transport_traits_wrapper2<TNativeTransport, p>& v)
{

}


}}