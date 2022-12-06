#pragma once

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/pbuf.h"
#include "embr/platform/lwip/udp.h"

#include "../fwd/transport.h"

namespace embr { namespace experimental {


// NOTE: Avoid effort into buffer_traits as we hope streambufs will carry that load
template <>
struct buffer_traits<pbuf> : tags::buf_chained
{
    typedef pbuf* native_type;

    static native_type next(native_type p)
    {
        return p->next;
    }

    static void* data(native_type p) { return p->payload; }
    static uint16_t size(native_type p) { return p->len; }
};



}}