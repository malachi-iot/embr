#include "../../dataport.h"
#include "transport.hpp"

#pragma once

namespace embr { namespace lwip {

/// Makes an lwip-specific UDP dataport
template <class TSubject>
auto make_udp_dataport(TSubject& s, uint16_t listen_port) -> 
    decltype(embr::make_dataport<experimental::UdpDataportTransport>(s, listen_port))
{
    return embr::make_dataport<experimental::UdpDataportTransport>(s, listen_port);
}


}}