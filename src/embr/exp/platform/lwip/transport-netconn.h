#pragma once

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/endpoint.h"
#include "embr/platform/lwip/streambuf.h"

// Has some issues and don't want to spend time fixing them on an experimental branch -
// we can fall back to non-wrapped true LwIP API
//#include "embr/platform/lwip/netconn.h"

#include "../fwd/transport.h"
#include "pbuf.h"
#include "netbuf.h"

namespace embr { namespace experimental {

template <>
struct transport_traits<netconn>
{
private:
    typedef struct netconn native_type;

public:
    typedef native_type* transport_type;
    typedef netbuf* ibuf_type;
    typedef netbuf* obuf_type;

    // System allocates netbuf here
    static void read(transport_type n, ibuf_type* new_buf)
    {

    }
};

}}