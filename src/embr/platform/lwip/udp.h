/**
 * 
 * References:
 * 
 * 1. https://lwip.fandom.com/wiki/Raw/UDP
 * 2. https://www.nongnu.org/lwip/2_0_x/group__udp__raw.html#gaa4546c43981f043c0ae4514d625cc3fc
 */
#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/udp.h>

}

#include <estd/internal/platform.h>
#include "../../internal/unique.h"
#include "endpoint.h"

namespace embr { namespace lwip { namespace udp {

// Wrapper for
// https://www.nongnu.org/lwip/2_0_x/structudp__pcb.html
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

    /**
     * @brief wrapper around udp_sendto
     * 
     * As per [1], "pbuf is not deallocated" as per [2]
     * 
     * @param pbuf 
     * @param addr 
     * @param port 
     * @return err_t 
     */
    err_t send(pbuf_pointer pbuf, 
        addr_pointer addr,
        uint16_t port)
    {
        return udp_sendto(pcb, 
            pbuf, 
            addr, 
            port);
    }

    // Experimental call permitting direct use of our Endpoint class
    // Not sure I want this level of complexity in what amounts to a PCB wrapper
    template <bool use_ptr>
    err_t send_experimental(pbuf_pointer pbuf,
        const embr::lwip::internal::Endpoint<use_ptr>& endpoint)
    {
        return send(pbuf,
            endpoint.address(),
            endpoint.port());
    }

    /**
     * @brief wrapper around udp_send
     * 
     * As per [1], "pbuf is not deallocated" as per [2]
     * 
     * @param pbuf 
     * @return err_t 
     */
    err_t send(pbuf_pointer pbuf)
    {
        return udp_send(pcb, pbuf);
    }

    /**
     * @brief wrapper around udp_recv
     * 
     * "Specifies a callback function that should be called when a UDP datagram
     * is received on the specified connection. The callback function is
     * responsible for deallocating the pbuf." [1]
     * 
     * @param recv_f 
     * @param recv_arg 
     */
    void recv(udp_recv_fn recv_f, void* recv_arg = NULLPTR)
    {
        udp_recv(pcb, recv_f, recv_arg);
    }

    /**
     * @brief 
     * 
     * connected only receives data from the connected address,
     * unconnected receive from any address
     * 
     * @param ipaddr 
     * @param port 
     * @return err_t 
     */
    err_t connect(addr_pointer ipaddr, u16_t port)
    {
        return udp_connect(pcb, ipaddr, port);
    }

    void disconnect() { udp_disconnect(pcb); }

    // DEBT: Consider removing this and expecting explicit 'udp_new'
    // from callers.  'create' is a murky maradigm (should it be static
    // or not?)
    inline static pcb_pointer create()
    {
        return udp_new();
    }

    bool alloc() { pcb = create(); return pcb != NULLPTR; }

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

    void local_port(uint16_t value)
    {
        pcb->local_port = value;
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

} // namespace udp

#if __has_cpp_attribute(deprecated)
[[deprecated("Use embr::lwip::udp::Pcb instead")]]
#endif
typedef udp::Pcb Pcb;

}

namespace experimental {

template <>
struct Unique<lwip::udp::Pcb> : lwip::udp::Pcb
{
    typedef lwip::udp::Pcb base_type;

    Unique() : base_type(create()) {}
    ~Unique() { base_type::free(); }

private:
    // stubs to disallow explicit memory management
    void alloc();
    void free();
};

}

}