#pragma once

// Forward declaration for LwIP itself
// FIX: Doesn't work
//typedef struct ip_addr_t;

// Due to above forward declaration not working, we are forced to include
// whole ip_addr.h header
extern "C" {

#include <lwip/ip_addr.h>

}


namespace embr { namespace lwip { namespace experimental {

// be advised, addr_pointer can go out of scope and get deallocated as per
// https://www.nongnu.org/lwip/2_1_x/udp_8h.html#af0ec7feb31acdb6e11b928f438c8a64b
template <bool use_address_ptr, class TAddress = ip_addr_t>
struct EndpointAddressBase;

template <bool use_address_ptr, class TAddress = ip_addr_t>
struct EndpointAddress;


}}}