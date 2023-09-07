#pragma once

#include <embr/platform/lwip/udp.h>

// https://www.nongnu.org/lwip/2_0_x/group__udp__raw.html

namespace embr { namespace experimental {
inline namespace v1 { inline namespace lwip { 

struct UdpPcb
{
    struct endpoint
    {
        const ip_addr_t* const ip;
        const uint16_t port;
    };

    struct mode_base
    {
        struct udp_pcb* pcb;

        void read(udp_recv_fn recv, void* recv_arg)
        {
            return udp_recv(pcb, recv, recv_arg);
        }

        err_t write(struct pbuf* p, endpoint e)
        {
            return udp_sendto(pcb, p, e.ip, e.port);
        }
    };
};

}

template <>
struct transport_traits<lwip::UdpPcb> : transport_traits_defaults
{
    static constexpr Support session = SUPPORT_SHOULD_NOT;
};


}
}}