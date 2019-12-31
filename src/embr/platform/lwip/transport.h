/**
 * @file
 * Higher level wrapper around pcb for UDP send operations
 */
#pragma once

#include "udp.h"
#include "streambuf.h"

namespace embr { namespace lwip { namespace experimental {

typedef const ip_addr_t* addr_pointer;

// be advised, addr_pointer can go out of scope and get deallocated as per
// https://www.nongnu.org/lwip/2_1_x/udp_8h.html#af0ec7feb31acdb6e11b928f438c8a64b
template <bool use_address_ptr>
struct EndpointAddress;

template <>
class EndpointAddress<true>
{
    addr_pointer _address;

public:
    EndpointAddress(addr_pointer _address) : _address(_address) {}

    addr_pointer address() const { return _address; }
};

template <>
class EndpointAddress<false>
{
    ip_addr_t _address;

public:
    EndpointAddress(addr_pointer _address) : _address(*_address) {}

    addr_pointer address() const { return &_address; }
};


template <bool use_address_ptr = true>
class Endpoint : public EndpointAddress<use_address_ptr>
{
    uint16_t _port;

public:
    Endpoint(addr_pointer address, uint16_t port) :
        EndpointAddress<use_address_ptr>(address),
        _port(port)
    {}

    uint16_t port() const { return _port; }
};


struct TransportBase
{
    typedef embr::lwip::experimental::addr_pointer addr_pointer;
    typedef struct pbuf* pbuf_pointer;
    typedef opbuf_streambuf ostreambuf_type;
    typedef ipbuf_streambuf istreambuf_type;
    typedef ostreambuf_type::netbuf_type netbuf_type;
};

template <bool use_address_ptr = true>
struct TransportUdp : TransportBase
{
    typedef Endpoint<use_address_ptr> endpoint_type;

    Pcb pcb;

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    TransportUdp(TArgs&& ...args) : pcb(std::forward<TArgs>(args)...) {}
#endif

#ifdef FEATURE_CPP_ALIASTEMPLATE
    template <class TChar>
    void send(basic_opbuf_streambuf<TChar>& streambuf, const endpoint_type& endpoint)
    {
        pcb.send(streambuf.netbuf().pbuf(), 
            endpoint.address(),
            endpoint.port());
    }
#endif

    void send(netbuf_type& netbuf, const endpoint_type& endpoint)
    {
        pcb.send(netbuf.pbuf(),
            endpoint.address(),
            endpoint.port());
    }
};



// interaction point for DataPort class which glues raw udp lwip to
// datapump.  DataPort is datagram/connectionless leaning
// FIX: As is often the case, clean up naming if we can
struct UdpDataportTransport : embr::lwip::experimental::TransportUdp<false>
{
    typedef embr::lwip::experimental::TransportUdp<false> base_type;
    typedef endpoint_type addr_t;
    // This is because DataPort reaches back in to deduce some things about
    // our transport netbuf/address structure
    typedef UdpDataportTransport transport_descriptor_t;

    // NOTE: Consider making the constructor what also receives the TSubject
    // and tag template class TSubject on data_recv itself
    // Consider also doing that with TDatapump and making actual queing done
    // in response to a notification
    // NOTE: Convention is that TDataPort* must always be first parameter
    template <class TDataPort>
    UdpDataportTransport(TDataPort* dataport, uint16_t port);

    template <class TDataPort>
    static void data_recv(void *arg, 
        struct udp_pcb *pcb, pbuf_pointer p,  
        addr_pointer addr, u16_t port);
};




}}}