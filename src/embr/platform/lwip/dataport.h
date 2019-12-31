#include "../../dataport.h"
#include "transport.hpp"

#pragma once

namespace embr { namespace lwip {

// TODO: do a non-decltype flavor of this
// FIX: fixup dummy_port (dummy listening port) issue, notice it's ignored
template <class TSubject>
auto make_udp_dataport(TSubject& s, uint16_t dummy_port) -> 
    decltype(embr::make_dataport<experimental::UdpDataportTransport>(s))
{
    return embr::make_dataport<experimental::UdpDataportTransport>(s);
}


}}