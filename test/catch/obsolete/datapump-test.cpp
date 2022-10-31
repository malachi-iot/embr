#include <catch.hpp>

#include <estd/string.h>

#include <embr/datapump.hpp>
#include <embr/exp/dataport-v2.h>
#include "datapump-test.h"

using namespace embr::experimental;

// specifically for v2 experimental datapump/dataport
struct FakeTransport
{
    typedef void* pbuf_type;
    typedef int addr_type;


};


// TODO: bring in fake_chrono from coap lib and plug it in here
// or maybe formalize it more into a 'manual_clock' which requires explicit
// external participation to push it forward.  Same arch as the fake chrono really
struct SyntheticRetry : BasicRetry<const char*, int>
{
    typedef BasicRetry<const char*, int> base_type;
    typedef typename base_type::datapump_item datapump_item;

    struct RetryItem : RetryItemBase<>
    {
        typedef RetryItemBase<> base_type;

        // helper method, not called by Datapump code
        int seq() { return this->pbuf[1] - '0'; }

        bool is_confirmable() { return this->pbuf[0] == 'C'; }

        bool is_acknowledge() { return this->pbuf[0] == 'A'; }

        // evaluate whether the incoming item is an ACK matching 'this' item
        // expected to be a CON.  Comparing against something without retry metadata
        // because a JUST RECEIVED ACK item won't have any retry metadata yet
        bool retry_match(RetryItem* compare_against)
        {
            // NOTE: so far have been proclaiming we do NOT check for proper ACK/CON here
            // as that is expected to be filtered elsewhere.  However, for unit test,
            // doing it here.  If we can localize that *entirely* here and phase out the is_xxx messages,
            // that would be better
            // TODO: Need to use the addr comparison specializer
            return compare_against->is_acknowledge() &&
                compare_against->seq() == seq() &&
                compare_against->addr == addr;
        }
    };

    /// @brief indicate that this item should be a part of retry list
    bool should_queue(RetryItem* item)
    {
        return item->counter() < 3;
    }
};


static const char* CON_0 = "C0hi2u"; // C = CON, 0 = sequence
static const char* ACK_0 = "A0"; // A = ACK, 0 = sequence
static const char* ACK_1 = "A1"; // A = ACK, 1 = sequence (won't match 0 from above)

TEST_CASE("datapump")
{
    synthetic_netbuf_type nb;
    synthetic_datapump dp;

    SECTION("A")
    {
        dp.enqueue_out(std::move(nb), -1);
        auto& item = dp.transport_front();

        REQUIRE(item.addr() == -1);
    }
    SECTION("v2 (experimental)")
    {
        using namespace embr::experimental;

        // Almost works, but messes up constructor for union scenario (const_string has no
        // default constructor)
        //typedef DatapumpWithRetry2<estd::layer2::const_string, int> datapump_type;
        typedef DatapumpWithRetry2<const char*, int> datapump_type;

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
            typedef Retry2<const char*, int, SyntheticRetry> retry_type;
            typedef retry_type::item_type item_type;
            retry_type retry;
            item_type item;

            item.addr = 0;
            item.pbuf = CON_0; // C = CON, 0 = sequence

            bool should_add = retry.evaluate_add_to_retry(&item);

            REQUIRE(should_add);

            REQUIRE(item.counter() == 1);
            REQUIRE(estd::distance(retry.retry_list.begin(), retry.retry_list.end()) == 1);

            // this 'item' contains a CON not an ACK, so we should get nothing for this
            item_type* to_remove = retry.evaluate_remove_from_retry(&item);

            REQUIRE(to_remove == NULLPTR);

            item_type ack_item;

            ack_item.addr = 0;

            ack_item.pbuf = ACK_1; // A = ACK, 1 = sequence (won't match 0 from above)

            // this 'item' contains an ACK, but sequence number doesn't match
            // note also undecided if ACK filtering should happen during this evaluate
            to_remove = retry.evaluate_remove_from_retry(&ack_item);

            REQUIRE(to_remove == NULLPTR);

            ack_item.pbuf = ACK_0;
            ack_item.addr = 1;

            // Now we should get a match, ACK seq 0 will match item's CON seq 0
            to_remove = retry.evaluate_remove_from_retry(&ack_item);

            // old item sitting in retry queue is now removed and returned
            REQUIRE(to_remove == NULLPTR);

            ack_item.addr = 0;

            // Now we should get a match, ACK seq 0 will match item's CON seq 0
            to_remove = retry.evaluate_remove_from_retry(&ack_item);

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

                dataport.received_from_transport("hi", 0, &context);
                dataport.process(&context);

                REQUIRE(context.state_progression_counter == 5);
            }
            SECTION("retry")
            {
                struct Context
                {
                    int state_progression_counter = 0;
                };

                // NOTE: May want to auto-initialize this to null, but for embedded scenarios that is
                // a smidgen of overhead that we might not need
                dataport.notifier = NULLPTR;

                dataport.send_to_transport(CON_0, 0);
                dataport.received_from_transport(ACK_0, 0);
            }
        }
    }
}
