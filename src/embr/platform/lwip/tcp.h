#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/tcp.h>

}

#include <estd/internal/platform.h>
#include "../../internal/unique.h"

#ifndef FEATURE_EMBR_LWIP_TCP_PCB_STRICT
#define FEATURE_EMBR_LWIP_TCP_PCB_STRICT 1
#endif

// Lots of ads, lots of information
// https://lwip.fandom.com/wiki/Raw/TCP

namespace embr { namespace lwip { namespace tcp {

template <bool auto_null = true>
class PcbBase
{
public:
    typedef struct tcp_pcb* pointer;

protected:
    pointer pcb;

public:
    // DEBT: account for auto_null in default constructor
    PcbBase() = default;
    PcbBase(pointer pcb) : pcb(pcb) {}
    PcbBase(pointer pcb, void* arg) : pcb(pcb)
    {
        tcp_arg(pcb, arg);
    }

    // DEBT: Not same convention that udp::Pcb used
    void create()
    {
        pcb = tcp_new();
    }

    void create(lwip_ip_addr_type type)
    {
        pcb = tcp_new_ip_type((uint8_t)type);
    }

    void abort()
    {
        tcp_abort(pcb);

        if(auto_null)   pcb = nullptr;
    }

    void accept(tcp_accept_fn accept) const
    {
        tcp_accept(pcb, accept);
    }

    err_t bind(const ip_addr_t* addr, uint16_t port) const
    {
        return tcp_bind(pcb, addr, port);
    }

    template <bool v>
    err_t bind(const embr::lwip::internal::Endpoint<v>& ep)
    {
        return tcp_bind(pcb, ep.address(), ep.port());
    }

    err_t close()
    {
        const err_t r = tcp_close(pcb);

        if(auto_null && r == ERR_OK) pcb = nullptr;

        return r;
    }

    err_t connect(const ip_addr_t* addr, uint16_t port, tcp_connected_fn connected)
    {
        return tcp_connect(pcb, addr, port, connected);
    }

    template <bool v>
    err_t connect(const embr::lwip::internal::Endpoint<v>& ep, tcp_connected_fn connected)
    {
        return tcp_connect(pcb, ep.address(), ep.port(), connected);
    }

    void arg(void* v) const
    {
        tcp_arg(pcb, v);
    }

    void err(tcp_err_fn efn)
    {
        tcp_err(pcb, efn);
    }

    bool listen(uint8_t backlog = TCP_DEFAULT_LISTEN_BACKLOG)
    {
        tcp_pcb* reallocated = tcp_listen_with_backlog(pcb, backlog);

        if(reallocated == nullptr) return false;

        pcb = reallocated;
        return true;
    }

    bool listen(uint8_t backlog, err_t* err)
    {
        tcp_pcb* reallocated = tcp_listen_with_backlog_and_err(pcb, backlog, err);

        if(reallocated == nullptr) return false;

        pcb = reallocated;
        return true;
    }

    err_t output()
    {
        return tcp_output(pcb);
    }

    ///
    /// @interval "TCP coarse grained timer shots, which typically occurs twice a second"
    /// https://www.nongnu.org/lwip/2_1_x/group__tcp__raw.html#gafba47015098ed7ce523dcf7bdf70f7e5
    void poll(tcp_poll_fn poll, uint8_t interval) const
    {
        tcp_poll(pcb, poll, interval);
    }

    void recv(tcp_recv_fn recv) const
    {
        tcp_recv(pcb, recv);
    }

    void recved(uint16_t len)
    {
        tcp_recved(pcb, len);
    }

    err_t shutdown(int shut_rx, int shut_tx)
    {
        const err_t r = tcp_shutdown(pcb, shut_rx, shut_tx);

        if(auto_null && r == ERR_OK) pcb = nullptr;

        return r;
    }

    constexpr uint16_t sndbuf() const
    {
        return tcp_sndbuf(pcb);
    }

    constexpr bool valid() const { return pcb != nullptr; }

    err_t write(const void* buf, uint16_t len, uint8_t flags) const
    {
        return tcp_write(pcb, buf, len, flags);
    }
};

using Pcb = PcbBase<FEATURE_EMBR_LWIP_TCP_PCB_STRICT>;

}}

namespace experimental {

template<>
struct Unique<lwip::tcp::Pcb> : lwip::tcp::PcbBase<true>
{
    using base_type = lwip::tcp::PcbBase<true>;

    Unique() : base_type(tcp_new()) {}

    template <bool v>
    Unique(const lwip::internal::Endpoint<v>& bind_to) :
        base_type(tcp_new())
    {
        base_type::bind(bind_to);
    }

    Unique(Unique&& move_from) : base_type{move_from}
    {
        move_from.pcb = nullptr;
    }
    ~Unique()
    {
        if(base_type::pcb)    base_type::close();
    }
};

}

}