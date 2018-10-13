#include <catch.hpp>

#include <estd/string.h>

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

        // Almost works, but messes up constructor for union scenario (const_string has no
        // default constructor)
        //typedef Datapump2<estd::layer2::const_string, int> datapump_type;
        typedef Datapump2<const char*, int> datapump_type;

        SECTION("raw datapump")
        {
            datapump_type datapump;

            datapump.enqueue_from_transport("test", 0);

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

            struct Context
            {
                int state_progression_counter = 0;
            };

            Context context;

            SECTION("basic callback")
            {
                static state_type state_progression[]
                        {
                                state_type::TransportInQueueing,
                                state_type::TransportInQueued,
                                state_type::TransportInDequeuing,
                                state_type::TransportInDequeued,
                                state_type::RetryEvaluating
                        };

                // Thinking notifier is where 3 things will happen:
                // - direct interaction with transport
                // - direct interaction with application entry point
                // - cleanup of no-longer-used pbufs
                // thinking also that these items are common enough that further framework support
                // for them would be nice, but also risky of overengineering so holding off for now
                dataport.notifier = [](state_type state, context_type* context) {
                    auto user = (Context*) context->user;
                    INFO(user->state_progression_counter);
                    REQUIRE(state_progression[user->state_progression_counter++] == state);

                    switch (state)
                    {
                        case state_type::TransportInDequeued:
                            estd::layer2::const_string s = context->item->pbuf;

                            REQUIRE(s == "hi");

                            break;
                    }
                };

                dataport.receive_from_transport("hi", 0, &context);
                dataport.process(&context);

                REQUIRE(context.state_progression_counter == 5);
            }
            SECTION("retry")
            {

            }
        }
    }
}
