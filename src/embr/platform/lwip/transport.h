/**
 * @file
 * Higher level wrapper around pcb for UDP send operations
 */
#pragma once

#include "features.h"
#include "endpoint.h"
#include "udp.h"
#include "streambuf.h"

namespace embr { namespace lwip { namespace experimental {

struct TransportBase
{
    typedef struct pbuf* pbuf_pointer;
#if FEATURE_EMBR_NETBUF_STREAMBUF
    typedef legacy::opbuf_streambuf ostreambuf_type;
    typedef legacy::ipbuf_streambuf istreambuf_type;
    typedef ostreambuf_type::netbuf_type buffer_type;
#else
    typedef upgrading::opbuf_streambuf ostreambuf_type;
    typedef upgrading::ipbuf_streambuf istreambuf_type;
    // DEBT: Stop-gap, almost definitely we'd prefer outsiders to use
    // actual stream and not the raw "buffer_type" (which has always mapped to
    // a pbuf gracefully, by design)
    typedef Pbuf buffer_type;
#endif
};

template <bool use_address_ptr = true>
struct TransportUdp : TransportBase
{
    typedef Endpoint<use_address_ptr> endpoint_type;

    lwip::udp::Pcb pcb;

    typedef struct udp_pcb* pcb_pointer;

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    TransportUdp(TArgs&& ...args) : pcb(std::forward<TArgs>(args)...) {}
#endif

    // Follows partially proven guidance to overcome udp_send side-effects
    void send_experimental(const PbufBase& pbuf, const endpoint_type& endpoint)
    {
        pbuf_pointer underlying = pbuf;
        struct pbuf saved = *underlying;

        pcb.send_experimental(underlying, endpoint);

        *underlying = saved;
    }


    void send(pbuf_pointer pbuf, const endpoint_type& endpoint)
    {
        pcb.send(pbuf,
            endpoint.address(),
            endpoint.port());
    }

#if FEATURE_EMBR_NETBUF_STREAMBUF
#ifdef FEATURE_CPP_ALIASTEMPLATE
    template <class TChar>
    void send(legacy::basic_opbuf_streambuf<TChar>& streambuf, const endpoint_type& endpoint)
    {
        pcb.send(streambuf.netbuf().pbuf(), 
            endpoint.address(),
            endpoint.port());
    }
#endif

    void send(buffer_type& netbuf, const endpoint_type& endpoint)
    {
        pcb.send(netbuf.pbuf(),
            endpoint.address(),
            endpoint.port());
    }
#else
    template <class TChar>
    void send(upgrading::basic_opbuf_streambuf<TChar>& streambuf, const endpoint_type& endpoint)
    {
        pcb.send(streambuf.pbuf(), 
            endpoint.address(),
            endpoint.port());
    }
#endif
};



// interaction point for DataPort class which glues raw udp lwip to
// datapump.  DataPort is datagram/connectionless leaning
// FIX: As is often the case, clean up naming if we can
struct UdpDataportTransport : embr::lwip::experimental::TransportUdp<false>
{
    typedef embr::lwip::experimental::TransportUdp<false> base_type;
    // This is because DataPort reaches back in to deduce some things about
    // our transport netbuf/address structure
    typedef UdpDataportTransport transport_descriptor_t;

    // NOTE: Consider making the constructor what also receives the TSubject
    // and tag template class TSubject on data_recv itself
    // Consider also doing that with TDatapump and making actual queing done
    // in response to a notification
    // NOTE: Convention is that TDataPort* must always be first parameter
    template <class TDataPort>
    UdpDataportTransport(TDataPort* dataport, uint16_t port,
        uint16_t sourcePort = 0);

private:
    template <class TDataPort>
    static void data_recv(void *arg, 
        struct udp_pcb *pcb, pbuf_pointer p,  
        addr_pointer addr, u16_t port);
};


// like dataport flavor , but decoupled to only directly fire off
// TransportReceived events
// NOTE: At this time I think dataport also fires off transport events.
// not bad, but could be confusing so be careful.  
// Doesn't need/expect DatapumpSubject to be used
struct UdpSubjectTransport : embr::lwip::experimental::TransportUdp<false>
{
    typedef embr::lwip::experimental::TransportUdp<false> base_type;

    // TODO: Probably move this upward into TransportUdp itself, or maybe even
    // Pcb
    template <class TSubject>
    static udp::Pcb recv(TSubject& subject, uint16_t port);

    // NOTE: Phase this out, be better do be more pcb-like and explicitly 
    // bind/recv
    // for a better transport abstraction
    template <class TSubject>
    UdpSubjectTransport(TSubject& subject, uint16_t port);

    UdpSubjectTransport() {}

private:
    template <class TSubject>
    static void data_recv(void *arg, 
        struct udp_pcb *pcb, pbuf_pointer p,  
        addr_pointer addr, u16_t port);
};


}}}