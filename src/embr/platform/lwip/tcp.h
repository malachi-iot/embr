#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/tcp.h>

}

#include <estd/internal/platform.h>

#ifndef FEATURE_EMBR_LWIP_TCP_PCB_STRICT
#define FEATURE_EMBR_LWIP_TCP_PCB_STRICT 1
#endif

namespace embr { namespace lwip { namespace tcp {

class Pcb
{
public:
    typedef struct tcp_pcb* pointer;

private:
    pointer pcb;

public:
    Pcb() = default;
    Pcb(pointer pcb) : pcb(pcb) {}
    Pcb(pointer pcb, void* arg) : pcb(pcb)
    {
        tcp_arg(pcb, arg);
    }

    void create()
    {
        pcb = tcp_new();
    }

    void create(uint8_t type)
    {
        pcb = tcp_new_ip_type(type);
    }

    void abort()
    {
        tcp_abort(pcb);
#if FEATURE_EMBR_LWIP_TCP_PCB_STRICT
        pcb = nullptr;
#endif
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

#if FEATURE_EMBR_LWIP_TCP_PCB_STRICT
        if(r == ERR_OK) pcb = nullptr;
#endif

        return r;
    }

    void arg(void* v) const
    {
        tcp_arg(pcb, v);
    }

    void err(tcp_err_fn efn)
    {
        tcp_err(pcb, efn);
    }

    bool listen(uint8_t backlog)
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

    constexpr uint16_t sndbuf() const
    {
        return tcp_sndbuf(pcb);
    }

    err_t write(const void* buf, uint16_t len, uint8_t flags) const
    {
        return tcp_write(pcb, buf, len, flags);
    }
};

}}}