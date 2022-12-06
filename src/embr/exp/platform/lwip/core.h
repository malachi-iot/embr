#pragma once

extern "C" {

#include <lwip/api.h>

}

#include "embr/platform/lwip/features.h"

#include "../fwd/transport.h"

namespace embr { namespace experimental {

template <>
transport_results unify_result(err_enum_t e)
{
    switch(e)
    {
        case ERR_OK: return transport_results::OK;
        default: return transport_results::Undefined;
    }
}


template <>
transport_results unify_result(err_t e)
{
    return unify_result((err_enum_t)e);
}


}}