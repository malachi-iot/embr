#pragma once

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/endpoint.h"
#include "embr/platform/lwip/streambuf.h"

// Has some issues and don't want to spend time fixing them on an experimental branch -
// we can fall back to non-wrapped true LwIP API
//#include "embr/platform/lwip/netconn.h"

#include "../fwd/transport.h"
#include "pbuf.h"

namespace embr { namespace experimental {

template <>
struct transport_traits<netconn>
{
private:
    typedef struct netconn native_type;
};

}}