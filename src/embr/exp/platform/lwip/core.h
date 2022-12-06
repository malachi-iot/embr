#pragma once

extern "C" {

#include <lwip/api.h>

}

#include "embr/platform/lwip/features.h"

#include "../fwd/transport.h"

namespace embr { namespace experimental {

namespace lwip {

typedef const ip_addr_t* addr_pointer;

struct endpoint
{
    addr_pointer addr;
    uint16_t port;
};


}



template <>
transport_results unify_result(err_enum_t e)
{
    switch(e)
    {
        case ERR_OK:        return transport_results::OK;
        case ERR_MEM:       return transport_results::Memory;
        case ERR_TIMEOUT:   return transport_results::Timeout;
        default:            return transport_results::Undefined;
    }
}


template <>
transport_results unify_result(err_t e)
{
    return unify_result((err_enum_t)e);
}


}}