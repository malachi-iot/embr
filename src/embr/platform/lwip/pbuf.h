/**
 * @file
 * 
 * Where netbuf-mk2 pbuf provider lives
 * 
 * Adapted from bronze-star PbufNetbufWrapper
 */

#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/udp.h>

}

#undef putchar
#undef puts
#undef putc

namespace embr { namespace lwip {

struct PbufNetbuf
{
    typedef struct pbuf* pbuf_type;
    typedef pbuf_type* pbuf_pointer;
};

}}