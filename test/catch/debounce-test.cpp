#include <catch.hpp>

#include <embr/detail/debounce.hpp>

using namespace embr::detail;

TEST_CASE("Debounce and friends state machine tests", "[debounce]")
{
    SECTION("undefined beginning state")
    {
        Debouncer d;
        bool r;

        r = d.time_passed(estd::chrono::milliseconds(10), true);

        REQUIRE(r);
    }
    SECTION("initial off state")
    {
        Debouncer d(Debouncer::Off);
        bool r;

        r = d.time_passed(estd::chrono::milliseconds(10), true);
        REQUIRE(r == false);

        r = d.time_passed(estd::chrono::milliseconds(30), false);
        REQUIRE(r == false);

        r = d.time_passed(estd::chrono::milliseconds(10), true);
        REQUIRE(r == false);

        r = d.time_passed(estd::chrono::milliseconds(30), false);
        REQUIRE(r == true);
    }
}