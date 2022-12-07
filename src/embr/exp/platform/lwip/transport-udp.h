#pragma once

// Potential v2 of lwip transport.h

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/endpoint.h"
#include "embr/platform/lwip/streambuf.h"
#include "embr/platform/lwip/pbuf.h"
#include "embr/platform/lwip/udp.h"

#include "core.h"
#include "pbuf.h"

namespace embr { namespace experimental {

template <>
struct protocol_traits<udp_pcb>
{
    typedef lwip_ip_addr_type type;

    static constexpr lwip_ip_addr_type def() { return IPADDR_TYPE_ANY; }
};


template <>
struct transport_traits<udp_pcb> :
    tags::read_callback,
    tags::read_transaction, // emulated
    tags::write_polling,
    tags::write_transaction, // emulated
    tags::datagram,
    tags::connection
{
private:
    typedef udp_pcb native_type;
    typedef pbuf* raw_pbuf;
    typedef const ip_addr_t* addr_pointer;
    typedef embr::lwip::PbufBase pbuf_type;

public:
    typedef embr::lwip::udp::Pcb pcb_type;
    typedef pcb_type transport_type;
    typedef pbuf_type obuf_type;
    typedef pbuf_type ibuf_type;
    typedef transport_result_wrapper<err_t> result_type;

    typedef lwip::endpoint endpoint_type;

    struct tuple
    {
        raw_pbuf pbuf;
        endpoint_type endpoint;
    };

    static pcb_type create() { return pcb_type::create(); }

    static void free(pcb_type pcb) { return pcb.free(); }

    static result_type connect(pcb_type p, const endpoint_type& e)
    {
        return p.connect(e.addr, e.port);
    }


    static void disconnect(pcb_type p)
    {
        p.disconnect();
    }


    struct endpoint_transaction
    {
        endpoint_type endpoint;
        raw_pbuf buf;

        obuf_type out() const { return buf; }
        ibuf_type in() const { return buf; }

        bool write_completed() const { return buf == nullptr; }
        bool read_completed() const { return buf != nullptr; }
    };


    static transport_results begin_write(pcb_type pcb, endpoint_transaction* t, const endpoint_type* endpoint = nullptr)
    {
        embr::lwip::PbufBase buf(128);
        if(endpoint)
            t->endpoint = *endpoint;
        t->buf = buf;
        return transport_results::OK;
    }


    static void end_write(pcb_type pcb, endpoint_transaction* t)
    {
        pcb.send(t->buf);
        pbuf_free(t->buf);
        t->buf = nullptr;
    }

    // sequential style - very technically a poll, but I tend to treat this as
    // fire and forget
    static result_type write(pcb_type pcb, obuf_type buffer)
    {
        return pcb.send(buffer);
    }

    // Fire and forget
    // DEBT: Except for return codes
    static result_type write(pcb_type pcb, obuf_type buffer, endpoint_type e)
    {
        return pcb.send(buffer, e.addr, e.port);
    }

    struct read_callback_type
    {
        pcb_type transport;
        ibuf_type in;
        endpoint_type endpoint;

        read_callback_type(pcb_type transport, ibuf_type in, const endpoint_type& endpoint) :
            transport{transport}, in{in}, endpoint{endpoint}
        {}
    };

    template <typename F>
    static void recv_fn(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
    {
        F& f = *(F*) arg;

        endpoint_type e{addr, port};
        read_callback_type rct{pcb, p, e};

        f(rct);
    }


    template <typename F, typename TContext>
    static void recv_fn_no_closure(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
    {
        F& f = *(F*) nullptr; // DEBT: This will cause nasty problems if not used precisely right
        auto context = (TContext*) arg;

        endpoint_type e{addr, port};
        read_callback_type rct{pcb, p, e};

        f(rct, context);
    }


    template <typename F>
    inline static void read(pcb_type pcb, F&& f)
    {
        pcb.recv(recv_fn<F>, &f);
        //pcb.recv(&transport_traits::recv_fn, (void*)&f);
    }


    template <typename F, class TContext>
    inline static void read(pcb_type pcb, F&& f, TContext* context)
    {
        pcb.recv(recv_fn_no_closure<F>, context);
        //pcb.recv(&transport_traits::recv_fn, (void*)&f);
    }

    template <typename F>
    inline static void begin_read(pcb_type pcb, endpoint_transaction* t, F&& signal)
    {
        read(pcb, [=](const read_callback_type& rct)
        {
            t->endpoint = rct.endpoint;
            t->buf = rct.in;
            signal(rct);
        });
    }


    inline static void begin_read(pcb_type pcb, endpoint_transaction* t)
    {
        read(pcb, [=](const read_callback_type& rct)
        {
            t->endpoint = rct.endpoint;
            t->buf = rct.in;
        });
    }


    static void next_read(pcb_type pcb, endpoint_transaction* t)
    {

    }

    static void end_read(pcb_type pcb, endpoint_transaction* t)
    {

    }
};




template <class TTraits>
struct Transport<udp_pcb, TTraits>
{
    typedef TTraits traits_type;
    typedef typename traits_type::transport_type transport_type;
    
    transport_type pcb;

    typedef embr::lwip::opbuf_streambuf ostreambuf_type;
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;

    using pcb_type = typename traits_type::pcb_type;
    using endpoint_type = typename traits_type::endpoint_type;
    using ibuf_type = typename traits_type::ibuf_type;

    struct read_callback_type : traits_type::read_callback_type
    {
        typedef typename traits_type::read_callback_type base_type;

        istreambuf_type& streambuf;
        Transport transport()
        {
            return Transport(base_type::transport);
        }

        read_callback_type(Transport transport, istreambuf_type& streambuf, const endpoint_type& endpoint) :
            base_type(transport.pcb, nullptr, endpoint),
            streambuf(streambuf)
        {
            
        }
    };

    //Transport() : pcb{pcb_type::create()} {}

//protected:
    Transport(transport_type native = nullptr) : pcb{native} {}

public:
    transport_results alloc_and_bind(uint16_t port)
    {
        // get new pcb
        if (!pcb.alloc()) {
            LWIP_DEBUGF(UDP_DEBUG, ("transport.alloc failed!\n"));
            return transport_results::Memory;
        }
        /* bind to any IP address on port */
        if (pcb.bind(port) != ERR_OK) {
            LWIP_DEBUGF(UDP_DEBUG, ("transport.bind failed!\n"));
            return transport_results::Memory;
        }

        return transport_results::OK;
    }

    void begin_read() {}
    void end_read() {}


    template <typename F, typename TContext>
    static void recv_fn_no_closure(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
    {
        F& f = *(F*) nullptr; // DEBT: This will cause nasty problems if not used precisely right
        auto context = (TContext*) arg;

        endpoint_type e{addr, port};
        istreambuf_type in(p, false);   // will auto-free p since it's not bumping reference
        Transport t(pcb);

        read_callback_type rct{t, in, e};

        f(rct, context);
    }

    template <typename F, typename TContext>
    void read(F&& f, TContext* context)
    {
        pcb.recv(recv_fn_no_closure<F, TContext>, context);
    }


    inline void write(ostreambuf_type& out, endpoint_type e)
    {
        out.shrink();
        traits_type::write(pcb, out.pbuf(), e);
    }
};


template <>
struct transport_traits2<udp_pcb, IPADDR_TYPE_ANY> : transport_traits<udp_pcb>
{

};


template <>
struct tuple_traits<transport_traits<udp_pcb>::tuple>
{
    typedef transport_traits<udp_pcb> transport_type;
    typedef typename transport_type::tuple tuple_type;
    typedef lwip::endpoint endpoint_type;

    const static endpoint_type& endpoint(const tuple_type& tuple)
    {
        return tuple.endpoint;
    }
};


}}