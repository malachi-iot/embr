#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/udp.h>

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
    Pcb(pointer pcb) : pcb(pcb) {}
};

}}}