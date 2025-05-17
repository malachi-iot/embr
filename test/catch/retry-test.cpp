#include <catch2/catch_all.hpp>

#include <embr/exp/v4/retry.h>

using namespace embr;

TEST_CASE("Reusable retry", "[retry]")
{
    SECTION("v4")
    {
        using namespace experimental;
        using retry_type = v4::Retry<v4::RetryImpl<5, int>>;
        retry_type retry;
        retry_type::tracked_type tracked1, tracked2;

        bool r;

        r = retry.track(0, std::move(tracked1));
        REQUIRE(r);
        // FIX: Unexpected behavior, 0 doesn't register
        // registered https://github.com/malachi-iot/estdlib/issues/111
        //REQUIRE(retry.size() == 1);
        r = retry.track(1, std::move(tracked2));
        REQUIRE(r);
        //REQUIRE(retry.size() == 2);

        //r = retry.ack_received(0);
        //REQUIRE(r);
        r = retry.ack_received(1);
        REQUIRE(r);
    }
}
