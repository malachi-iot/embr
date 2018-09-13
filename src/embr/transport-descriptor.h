/**
 * @file
 */
#pragma once

namespace embr {

// bundle these two together, since they are paired all over the darned place
template <class TNetBuf, class TAddr>
struct TransportDescriptor
{
    typedef TNetBuf netbuf_t;
    typedef TAddr addr_t;
};



}
