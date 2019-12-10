#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/udp.h>

}

#include <estd/internal/platform.h>

namespace embr { namespace lwip {

// NOTE: Alternate universe version of udh.hpp/lwipcpp's UDP struct
struct Pcb
{
    typedef struct udp_pcb* pcb_pointer;
    typedef struct pbuf* pbuf_pointer;
    typedef const ip_addr_t* addr_pointer;

private:
    pcb_pointer pcb;
public:
    Pcb(pcb_pointer pcb) : pcb(pcb) {}

    err_t send(pbuf_pointer pbuf, 
        addr_pointer addr,
        uint16_t port)
    {
        return udp_sendto(pcb, 
            pbuf, 
            addr, 
            port);
    }

    void recv(udp_recv_fn recv_f, void* recv_arg)
    {
        udp_recv(pcb, recv_f, recv_arg);
    }

    // connected only receives data from the connected address,
    // unconnected receive from any address
    err_t connect(ip_addr_t* ipaddr, u16_t port)
    {
        return udp_connect(pcb, ipaddr, port);
    }

    void disconnect() { udp_disconnect(pcb); }
};

}}