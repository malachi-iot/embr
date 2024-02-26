#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/tcp.h>

}

#include <estd/internal/platform.h>

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

    void accept(tcp_accept_fn accept) const
    {
        tcp_accept(pcb, accept);
    }

    err_t bind(const ip_addr_t* addr, uint16_t port) const
    {
        return tcp_bind(pcb, addr, port);
    }

    err_t close()
    {
        return tcp_close(pcb);
    }

    void arg(void* v) const
    {
        tcp_arg(pcb, v);
    }

    void err(tcp_err_fn efn)
    {
        tcp_err(pcb, efn);
    }

    err_t output()
    {
        return tcp_output(pcb);
    }

    void recv(tcp_recv_fn recv)
    {
        tcp_recv(pcb, recv);
    }

    void recved(uint16_t len)
    {
        tcp_recved(pcb, len);
    }

    err_t write(const void* buf, uint16_t len, uint8_t flags) const
    {
        return tcp_write(pcb, buf, len, flags);
    }
};

}}}