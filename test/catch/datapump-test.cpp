#include <catch.hpp>

#include <estd/string.h>

#include <embr/datapump.hpp>
#include <embr/exp/datapump-v2.h>
#include "datapump-test.h"

using namespace embr::experimental;

// specifically for v2 experimental datapump/dataport
struct FakeTransport
{
    typedef void* pbuf_type;
    typedef int addr_type;


};

namespace embr { namespace experimental {

template<>
struct pbuf_traits<const char*>
{
    static estd::const_buffer raw(const char* pbuf)
    {
        return estd::const_buffer((const uint8_t*)pbuf, 100);
    }
};

}}


// TODO: bring in fake_chrono from coap lib and plug it in here
struct SyntheticRetry : BasicRetry<const char*, int>
{
    typedef BasicRetry<const char*, int> base_type;
    typedef typename base_type::datapump_item datapump_item;

    struct RetryItem : RetryItemBase<>
    {
        // helper method, not called by Datapump code
        static int seq(datapump_item& item) { return item.pbuf[1] - '0'; }

        static bool is_confirmable(datapump_item& item) { return item.pbuf[0] == 'C'; }

        static bool is_acknowledge(datapump_item& item) { return item.pbuf[0] == 'A'; }

        // evaluate whether the incoming item is an ACK matching 'this' item
        // expected to be a CON.  Comparing against something without retry metadata
        // because a JUST RECEIVED ACK item won't have any retry metadata yet
        bool retry_match(datapump_item* _this, datapump_item* compare_against)
        {
            // NOTE: so far have been proclaiming we do NOT check for proper ACK/CON here
            // as that is expected to be filtered elsewhere.  However, for unit test,
            // doing it here.  If we can localize that *entirely* here and phase out the is_xxx messages,
            // that would be better
            return is_acknowledge(*compare_against) &&
                seq(*compare_against) == seq(*_this);
        }
    };

    /// @brief indicate that this item should be a part of retry list
    bool should_queue(RetryItem* item, datapump_item* _item)
    {
        return item->counter < 3;
    }
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
        SECTION("retry")
        {
            typedef Datapump2<const char*, int, SyntheticRetry> datapump_retry_type;
            typedef datapump_retry_type::Item item_type;
            datapump_retry_type datapump;
            item_type item;

            item.pbuf = "C0hi2u"; // C = CON, 0 = sequence

            datapump.evaluate_add_to_retry(&item);

            REQUIRE(item.counter == 1);
            REQUIRE(estd::distance(datapump.retry_list.begin(), datapump.retry_list.end()) == 1);

            // this 'item' contains a CON not an ACK, so we should get nothing for this
            item_type* to_remove = datapump.evaluate_remove_from_retry(&item);

            REQUIRE(to_remove == NULLPTR);

            item_type ack_item;

            ack_item.pbuf = "A1"; // A = ACK, 1 = sequence (won't match 0 from above)

            // this 'item' contains an ACK, but sequence number doesn't match
            // note also undecided if ACK filtering should happen during this evaluate
            to_remove = datapump.evaluate_remove_from_retry(&ack_item);

            REQUIRE(to_remove == NULLPTR);

            ack_item.pbuf = "A0";

            // Now we should get a match, ACK seq 0 will match item's CON seq 0
            to_remove = datapump.evaluate_remove_from_retry(&ack_item);

            // old item sitting in retry queue is now removed and returned
            REQUIRE(to_remove == &item);
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
        }
    }
}
