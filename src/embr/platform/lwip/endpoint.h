#pragma once

#include "fwd.h"

// This grabs all the goodies needed for ESTD_CPP_FORWARDING_CTOR
#include <estd/utility.h>

namespace embr { namespace lwip { namespace experimental {

typedef const ip_addr_t* addr_pointer;

// DEBT: Consolidate with value_evaporator's instance_provider & pointer_from_instance_provider

template <class TAddress>
class EndpointAddressBase<true, TAddress>
{
public:
    typedef TAddress value_type;
    typedef const value_type* pointer;

private:    
    pointer address_;

public:
    EndpointAddressBase(pointer address_) : address_(address_) {}

    pointer address() const { return address_; }
};

template <class TAddress>
class EndpointAddressBase<false, TAddress>
{
public:
    typedef TAddress value_type;
    typedef const value_type* pointer;

private:
    const value_type address_;

public:
    EndpointAddressBase(pointer address_) : address_(*address_) {}

    pointer address() const { return &address_; }
};

template <bool use_pointer>
class EndpointAddress<use_pointer, ip_addr_t> :
    public EndpointAddressBase<use_pointer, ip_addr_t>
{
    typedef EndpointAddressBase<use_pointer, ip_addr_t> base_type;

public:
    ESTD_CPP_FORWARDING_CTOR(EndpointAddress);

    lwip_ip_addr_type type() const
    {
        return (lwip_ip_addr_type) IP_GET_TYPE(base_type::address());
    }
};

// Only enable explicit ip4_addr_t if IPv6 is available.  This may seem
// backwards.  This is needed because ip_addr_t aliases to ip4_addr_t
// when IPv6 is not present.  That means above ip_addr_t does double
// duty for ip4_addr_t and ip_addr_t and would cause a duplicate/conflict
// here
// Nice to have distinct ip4_addr_t for scenarios where you have IPv6
// compiled in but only want to support IPv4 in that instance (likely for
// overhead reasons)
#if LWIP_IPV6
template <bool use_pointer>
class EndpointAddress<use_pointer, ip4_addr_t> :
    public EndpointAddressBase<use_pointer, ip4_addr_t>
{
    typedef EndpointAddressBase<use_pointer, ip4_addr_t> base_type;

public:
    ESTD_CPP_FORWARDING_CTOR(EndpointAddress);

    lwip_ip_addr_type type() const
    {
        return IPADDR_TYPE_V4;
    }
};
#endif

}

namespace internal {


template <bool use_address_ptr = true>
class Endpoint : public embr::lwip::experimental::EndpointAddress<use_address_ptr>
{
    typedef embr::lwip::experimental::EndpointAddress<use_address_ptr> base_type;

    const uint16_t port_;

public:
    typedef typename base_type::pointer addr_pointer;

    Endpoint(addr_pointer address, uint16_t port) :
        base_type(address),
        port_(port)
    {}

    uint16_t port() const { return port_; }
};


// DEBT: Add !=
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

//typedef experimental::Endpoint<> Endpoint;

}}