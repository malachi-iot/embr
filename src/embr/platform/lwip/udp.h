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

private:
    pcb_pointer pcb;
public:
    Pcb(pcb_pointer pcb) : pcb(pcb) {}

    void sendto(pbuf_pointer pbuf, 
        const ip_addr_t* addr,
        uint16_t port)
    {
        udp_sendto(pcb, 
            pbuf, 
            addr, 
            port);
    }
};

}}