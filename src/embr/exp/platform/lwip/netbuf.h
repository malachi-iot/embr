#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/netbuf.h>

}

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/pbuf.h"
#include "embr/platform/lwip/udp.h"

#include "../fwd/transport.h"

namespace embr { namespace experimental {


// NOTE: Avoid effort into buffer_traits as we hope streambufs will carry that load

template <>
struct buffer_traits<netbuf> :
    tags::buf_runtime_allocated,
    tags::buf_chained   // FIX: Not entirely sure chaining here has same meaning as with pbuf
{

};

}}