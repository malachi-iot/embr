#include <catch.hpp>

#include <embr/datapump.hpp>
#include <embr/dataport.hpp>
#include <embr/observer.h>
#include "datapump-test.h"

using namespace embr;

struct synthetic_context
{
    int value;
};


// Renamed from StatefulObserver because compiler picked it up as the same class as
// from observer-test --- seems like a bug!
struct DataportStatefulObserver
{
    int counter;

    DataportStatefulObserver() : counter(0) {}

    typedef DataPortEvents<synthetic_datapump> event;

    void on_notify(event::receive_queued e, synthetic_context& c)
    {

    }

    void on_notify(event::receive_queued e)
    {

    }

    void on_notify(event::send_queued e)
    {

    }
};

struct synthetic_transport
{
    typedef synthetic_transport_descriptor transport_descriptor_t;

    // FIX: dummy initializer to accomodate current coap hard-wiredness
    synthetic_transport(void*, int) {}

    bool recv(synthetic_netbuf_type**, int*) { return false; }
    void send(synthetic_netbuf_type&, int) {}
};


TEST_CASE("dataport")
{
    synthetic_netbuf_type nb;

    SECTION("synthetic dataport")
    {
        // make a 1-count subject/observer binding
        auto s = layer1::make_subject(DataportStatefulObserver());

        typedef DataPort<synthetic_datapump, synthetic_transport, decltype (s)&> synthetic_dataport;

        synthetic_dataport dp(s);
        synthetic_context c;

        WHEN("receiving")
        {
            dp.enqueue_from_receive(std::move(nb), 0);
        }
        WHEN("sending")
        {
            dp.enqueue_for_send(std::move(nb), 1);
        }

        dp.service();
    }
    SECTION("make_dataport")
    {
        auto s = void_subject();

        auto dataport = embr::make_dataport<synthetic_transport>(s);
    }
}
