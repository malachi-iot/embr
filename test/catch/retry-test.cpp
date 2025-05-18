#include <catch2/catch_all.hpp>

#include <embr/exp/v4/retry.h>

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
        using retry_type = v4::Retry<v4::RetryImpl<5, int>>;
        using tp = retry_type::clock_type::time_point;
        retry_type retry;
        retry_type::tracked_type tracked1, tracked2;
        retry_type::pointer item;

        bool r;

        item = retry.track(2, tp(1s), std::move(tracked1));
        REQUIRE(item);
        // FIX: Unexpected behavior, 0 doesn't register
        // registered https://github.com/malachi-iot/estdlib/issues/111
        //REQUIRE(retry.size() == 1);
        item = retry.track(1, tp(2s), std::move(tracked2));
        REQUIRE(item);
        REQUIRE(retry.size() == 2);

        item = retry.top();
        REQUIRE(item);
        //REQUIRE(item->last_attempt_ == 1);

        //r = retry.is_ready(tp(500ms));
        //REQUIRE(r == false);

        r = retry.ack_received(2);
        REQUIRE(r);
        r = retry.ack_received(1);
        REQUIRE(r);

        //r = retry.is_ready(tp(500ms));
        //REQUIRE(r == false);

        r = retry.is_ready(tp(2s));
        REQUIRE(r);

        r = retry.untrack();
        REQUIRE(r);
    }
}
