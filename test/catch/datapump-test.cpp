#include <catch.hpp>

#include <embr/datapump.hpp>
#include <embr/exp/datapump-v2.h>
#include "datapump-test.h"

// specifically for v2 experimental datapump/dataport
struct FakeTransport
{
    typedef void* pbuf_type;
    typedef int addr_type;


};

TEST_CASE("datapump")
{
    synthetic_netbuf_type nb;
    synthetic_datapump dp;

    SECTION("A")
    {

    }
    SECTION("v2 (experimental)")
    {
        using namespace embr::experimental;

        typedef Datapump2<void*, int> datapump_type;

        SECTION("raw datapump")
        {
            datapump_type datapump;

            datapump.enqueue_from_transport((void*) "test", 0);

            REQUIRE(datapump.from_transport_ready());

            auto item = datapump.dequeue_from_transport();

            REQUIRE(item->addr == 0);

            datapump.deallocate(item);
        }
        SECTION("dataport")
        {
            typedef Dataport2<datapump_type> dataport_type;
            dataport_type dataport;
            typedef dataport_type::State state_type;
            typedef dataport_type::NotifyContext context_type;

            static state_type state_progression[]
            {
                state_type::TransportInQueueing,
                state_type::TransportInQueued,
                state_type::TransportInDequeuing,
                state_type::TransportInDequeued,
                state_type::RetryEvaluating
            };

            struct Context
            {
                int state_progression_counter = 0;

                void evalutate()
                {

                }
            };

            Context context;

            dataport.notifier = [](state_type state, context_type* context)
            {
                auto user = (Context*) context->user;
                INFO(user->state_progression_counter);
                REQUIRE(state_progression[user->state_progression_counter++] == state);
            };

            dataport.receive_from_transport((void*)"hi", 0, &context);
            dataport.process(&context);

            REQUIRE(context.state_progression_counter == 5);
        }
    }
}
