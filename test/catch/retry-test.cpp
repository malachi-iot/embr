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
        using retry_type = v4::Retry<v4::RetryImpl<5, estd::layer1::optional<int16_t, -1>, tracked_type>>;
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
            // To register 0 we need: https://github.com/malachi-iot/estdlib/issues/111
            REQUIRE(retry.size() == 1);
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

            REQUIRE(retry.size() == 2);

            item = retry.top();
            REQUIRE(item->first == 2);
            REQUIRE(item->second.as_string() == "hello #2");

            retry.track(0, tp(3s));
            REQUIRE(retry.size() == 3);
        }
        SECTION("send 2 ack 1")
        {
            using pointer = retry_type::pointer;
            auto poller = [](pointer p)
            {
                // 'retrack' auto increments retry count.  There might be edge cases where we want
                // more control over that
                if(p->second.attempt_count_ == 2) return false;

                p->second.next_attempt_ += 250ms;

                return true;
            };

            pointer item1 = retry.track(1, tp(500ms));
            ready = retry.ready(tp(250ms));
            REQUIRE(!ready);

            // NOTE: Underlying mechanism doesn't kick back duplicates yet, so instead it silently fails
            pointer item2 = retry.track(2, tp(1000ms));

            ready = retry.ready(tp(500ms));
            REQUIRE(ready);
            REQUIRE(ready == item1);
            retry.poll_one(tp(500ms), poller);
            REQUIRE(item1->second.attempt_count_ == 1);
            REQUIRE(item1->second.next_attempt_ == tp(750ms));
            ready = retry.ready(tp(1000ms));
            REQUIRE(ready == item1);
            REQUIRE(item1->second.attempt_count_ == 1);
            retry.poll_one(tp(750ms), poller);
            REQUIRE(item1->second.attempt_count_ == 2);
            REQUIRE(item2->second.attempt_count_ == 0);
            retry.poll_one(tp(1000ms), poller);
            retry.poll_one(tp(1000ms), poller);     // #1 expires
            REQUIRE(retry.size() == 1);
            REQUIRE(item1->second.attempt_count_ == 2);
            REQUIRE(item2->second.attempt_count_ == 1);
            retry.ack_received(2);
#if FEATURE_EMBR_RETRY_V4_ACK_IS_GC == 0
            REQUIRE(retry.size() == 1);
#endif
            retry.poll(tp(1250ms), poller);
            REQUIRE(retry.size() == 0);
        }
        SECTION("automated")
        {

        }
        SECTION("array key")
        {
            using mac = estd::array<uint8_t, 6>;
            using hasher = estd::internal::container_hash<uint32_t>;
            using retry_type = v4::Retry<v4::RetryImpl<5, mac, tracked_type, hasher>>;
            retry_type retry;

            mac broadcast{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

            // Some kind of failure with equal_to functor
            //retry.track(broadcast, tp(500ms));
        }
    }
}
