#pragma once

#include <estd/internal/value_evaporator.h>

#include <embr/scheduler.h>

// Where's retry v3?  That's under platform/freertos/exp/transport-retry

namespace embr { namespace experimental { namespace mk4 {

template <class TTransport>
struct RetryManager :
    estd::internal::struct_evaporator<TTransport>
{
    typedef estd::internal::struct_evaporator<TTransport> transport_provider;
    typedef typename transport_provider::evaporated_type transport_type;

    transport_type transport() { return transport_provider::value(); }
};

}}}