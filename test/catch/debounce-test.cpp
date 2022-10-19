#include <catch.hpp>

#include <embr/detail/debounce.hpp>

using namespace embr::detail;

TEST_CASE("Debounce and friends state machine tests", "[debounce]")
{
    SECTION("basic")
    {
        Debouncer d;

        d.time_passed(estd::chrono::milliseconds(10));
    }
}