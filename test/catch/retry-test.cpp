#include <catch2/catch_all.hpp>

#include <estd/string.h>

#include <embr/exp/v4/retry.h>

#include "retry-test.h"
#include "test-data.h"

using namespace embr;

TEST_CASE("Reusable retry", "[retry]")
{
    // TODO: test against nulled-out entries in the middle of priority queue how that impacts
    // pushes AND pops

    SECTION("v4")
    {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        using namespace experimental;
        using tracked_type = v4::ReferenceTracked<32>;
        using retry_type = v4::Retry<v4::RetryImpl<5, int, tracked_type>>;
        using tp = retry_type::clock_type::time_point;
        retry_type retry;
        retry_type::tracked_type tracked1, tracked2;
        retry_type::pointer item, ready;

        bool r;

        SECTION("basics")
        {
            item = retry.track(2, tp(1s), std::move(tracked1));
            REQUIRE(item);
            item->second.as_string() = "hello #2";
            // FIX: Unexpected behavior, 0 doesn't register
            // registered https://github.com/malachi-iot/estdlib/issues/111
            //REQUIRE(retry.size() == 1);
            item = retry.track(1, tp(2s), std::move(tracked2));
            item->second.as_string() = "hello #1";
            REQUIRE(item);
            REQUIRE(retry.size() == 2);

            item = retry.top();
            REQUIRE(item);
            //REQUIRE(item->last_attempt_ == 1);

            r = retry.ready(tp(500ms));
            REQUIRE(r == false);

            //r = retry.ack_received(2);
            //REQUIRE(r);
            r = retry.ack_received(1);
            REQUIRE(r);

            r = retry.ready(tp(500ms));
            REQUIRE(r == false);

            r = retry.ready(tp(2s));
            REQUIRE(r);

            // Still have endpoint #2 tracked
            r = retry.untrack();
            REQUIRE(r == false);

            REQUIRE(retry.size() == 1);

            item = retry.top();
            REQUIRE(item->first == 2);
            REQUIRE(item->second.as_string() == "hello #2");
        }
        SECTION("send 2 ack 2")
        {
            using pointer = retry_type::pointer;
            auto poller = [](pointer p)
            {
                if(++p->second.retry_count_ == 2) return false;

                p->second.next_attempt_ += 250ms;

                return true;
            };

            item = retry.track(1, tp(500ms));
            ready = retry.ready(tp(250ms));
            REQUIRE(!ready);

            // NOTE: Underlying mechanism doesn't kick back duplicates yet, so instead it silently fails
            item = retry.track(2, tp(1000ms));
            // FIX: Need to sort by greater, not less, on times
            ready = retry.ready(tp(500ms));
            REQUIRE(ready);
            retry.poll(tp(500ms), poller);
            ready = retry.ready(tp(1000ms));
            REQUIRE(ready);
            retry.poll(tp(750ms), poller);
            retry.poll(tp(1000ms), poller);
            retry.poll(tp(1001ms), poller);
        }
        SECTION("automated")
        {

        }
    }
}
