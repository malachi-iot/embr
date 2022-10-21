#include <catch.hpp>

#include <embr/detail/debounce.hpp>

#include <embr/exp/filter.h>

using namespace embr::detail;

namespace embr { namespace experimental {


//typedef fake_lpf<estd::chrono::milliseconds, 10> fake_lpf_ms;
typedef filter_wrapper<estd::chrono::milliseconds> fake_lpf_ms;


}}

TEST_CASE("Debounce and friends state machine tests", "[debounce]")
{
    SECTION("undefined beginning state")
    {
        Debouncer d;
        bool r;

        r = d.time_passed(estd::chrono::milliseconds(10), true);

        REQUIRE(r == false);
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
    SECTION("expire window")
    {
        
    }
    SECTION("experimental")
    {
        using namespace embr::experimental;

        SECTION("lpf")
        {
            //fake_lpf_ms::filter<30>;
            fake_lpf<int, 30> f;

            f.add(10);
        }
        SECTION("wrapped")
        {

        }
    }
}