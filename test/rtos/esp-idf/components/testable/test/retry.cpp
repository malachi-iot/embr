#include <embr/platform/freertos/exp/transport-retry.h>
#include <embr/platform/lwip/iostream.h>
#include <embr/platform/lwip/transport.h>

#include "unity.h"

#include "esp_log.h"

struct RetryImpl
{
    typedef embr::lwip::experimental::TransportUdp<> transport_type;
    typedef typename transport_type::endpoint_type endpoint_type;
    typedef int key_type;
    typedef TickType_t timebase_type;

    struct item_policy_impl_type
    {
        void process_timeout() {}

        timebase_type get_new_expiry()
        {
            return 100;
        }

        bool retry_done() const { return true; }
    };

    // whole reason we have an explicit match instead of implicit match, so we can
    // match just on address rather than address + port (in other words, an app
    // specific kind of address match)
    bool match(endpoint_type incoming_endpoint, endpoint_type tracked_endpoint)
    {
        return incoming_endpoint.address() == tracked_endpoint.address();
    }
};

using namespace embr::lwip;

// FIX: Duplicate/redundant with experimental.cpp - however this here
// is the authoratative version
TEST_CASE("experimental retry code", "[exp-retry]")
{
    typedef embr::lwip::experimental::TransportUdp<> transport_type;
    typedef embr::experimental::RetryManager<transport_type, RetryImpl> retry_manager_type;
    transport_type::endpoint_type synthetic_endpoint(nullptr, 0);

    retry_manager_type rm;
    
    opbufstream out(256);

    out << "hi2u" << estd::endl;

    rm.send(*out.rdbuf(), synthetic_endpoint, 7);
}