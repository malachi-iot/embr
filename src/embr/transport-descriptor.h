/**
 * @file
 */
#pragma once

namespace embr {

// bundle these two together, since they are paired all over the darned place
template <class TNetBuf, class TEndpoint>
struct TransportDescriptor
{
    typedef TNetBuf netbuf_type;
    typedef TEndpoint endpoint_type;
};



}
