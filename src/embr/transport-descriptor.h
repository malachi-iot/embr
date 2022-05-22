/**
 * @file
 */
#pragma once

namespace embr {

// bundle these two together, since they are paired all over the darned place
///
/// \tparam TBuffer Low level buffer which transport natively uses
/// \tparam TEndpoint Full address - e.g. for IPv4 has 4 digit addr plus port
template <class TBuffer, class TEndpoint>
struct TransportDescriptor
{
    typedef TBuffer buffer_type;
    typedef TEndpoint endpoint_type;
};



}
