#pragma once

namespace embr { namespace lwip { namespace experimental {

// be advised, addr_pointer can go out of scope and get deallocated as per
// https://www.nongnu.org/lwip/2_1_x/udp_8h.html#af0ec7feb31acdb6e11b928f438c8a64b
template <bool use_address_ptr>
struct EndpointAddress;


}}}