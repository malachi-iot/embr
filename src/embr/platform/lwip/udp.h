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
    // NOTE: Do not like allowing this to init without having an
    // actual pcb - however, low level UDP kind of wants to do this
    // because we often see a udp_new without a udp_remove.  Remember,
    // our Pcb is a wrapper around udp_pcb* --NOT-- udp_pcb
    Pcb(pcb_pointer pcb = NULLPTR) : pcb(pcb) {}

    bool has_pcb() const { return pcb != NULLPTR; }

    err_t send(pbuf_pointer pbuf, 
        addr_pointer addr,
        uint16_t port)
    {
        return udp_sendto(pcb, 
            pbuf, 
            addr, 
            port);
    }

    // as per https://www.nongnu.org/lwip/2_0_x/group__udp__raw.html#gaa4546c43981f043c0ae4514d625cc3fc
    err_t send(pbuf_pointer pbuf)
    {
        return udp_send(pcb, pbuf);
    }

    void recv(udp_recv_fn recv_f, void* recv_arg = NULLPTR)
    {
        udp_recv(pcb, recv_f, recv_arg);
    }

    // connected only receives data from the connected address,
    // unconnected receive from any address
    err_t connect(addr_pointer ipaddr, u16_t port)
    {
        return udp_connect(pcb, ipaddr, port);
    }

    void disconnect() { udp_disconnect(pcb); }

    bool alloc() { pcb = udp_new(); return pcb != NULLPTR; }

    bool alloc(lwip_ip_addr_type type)
    {
        pcb = udp_new_ip_type(type);
        return pcb != NULLPTR;
    }

    void free() { udp_remove(pcb); }

    err_t bind(addr_pointer ipaddr, u16_t port)
    {
        return udp_bind(pcb, ipaddr, port);
    }

    err_t bind(u16_t port)
    {
        return bind(IP_ADDR_ANY, port);
    }
};

namespace experimental {

// auto allocates and frees itself
// not so sure about the naming
struct AutoPcb : Pcb
{
    AutoPcb() : Pcb(udp_new())
    {
    }

    ~AutoPcb()
    {
        free();
    }
};

}

}}