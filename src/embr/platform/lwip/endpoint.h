#pragma once

extern "C" {

#include <lwip/ip_addr.h>

}

namespace embr { namespace lwip { namespace experimental {

typedef const ip_addr_t* addr_pointer;

// be advised, addr_pointer can go out of scope and get deallocated as per
// https://www.nongnu.org/lwip/2_1_x/udp_8h.html#af0ec7feb31acdb6e11b928f438c8a64b
template <bool use_address_ptr>
struct EndpointAddress;

template <>
class EndpointAddress<true>
{
    addr_pointer _address;

public:
    EndpointAddress(addr_pointer _address) : _address(_address) {}

    addr_pointer address() const { return _address; }
};

template <>
class EndpointAddress<false>
{
    ip_addr_t _address;

public:
    EndpointAddress(addr_pointer _address) : _address(*_address) {}

    addr_pointer address() const { return &_address; }
};


template <bool use_address_ptr = true>
class Endpoint : public EndpointAddress<use_address_ptr>
{
    uint16_t _port;

public:
    Endpoint(addr_pointer address, uint16_t port) :
        EndpointAddress<use_address_ptr>(address),
        _port(port)
    {}

    uint16_t port() const { return _port; }
};


// DEBT: Not well tested
template <bool use_address_ptr_lhs, bool use_address_ptr_rhs>
bool operator ==(
    const Endpoint<use_address_ptr_lhs>& lhs,
    const Endpoint<use_address_ptr_rhs>& rhs)
{
    // *lhs.address() == *rhs.address();
    // We need an ipv4 vs ipv6 version to properly compare addresses
    bool address_match = ip_addr_cmp(lhs.address(), rhs.address());

    return lhs.port() == rhs.port() && address_match;
}


}

typedef experimental::Endpoint<> Endpoint;

}}