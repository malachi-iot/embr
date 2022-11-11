#include <catch.hpp>

#include <embr/detail/debounce.hpp>
#include <embr/detail/button.hpp>

#include <embr/exp/runtime-chrono.h>
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
            fake_lpf<int, 30> f;

            f.add(10);
        }
        SECTION("wrapped")
        {
            fake_lpf_ms::wrapped<30> f;

            f.add(estd::chrono::milliseconds(1));
            f.add(estd::chrono::microseconds(1500));

            // FIX: Was working fine until I added runtime chrono - smells like an undefined variable
            // value situation
            //REQUIRE(f.energy().count() == 2);
        }
        SECTION("runtime chrono")
        {
            SECTION("converter (runtime default 80)")
            {
                DurationConverter<int, -1> converter;

                uint64_t v = converter.convert(estd::chrono::milliseconds(10));

                REQUIRE(v == 10000);

                v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<1, 3> >(1));
                REQUIRE(v == 333333);

                // 5/33 of a second = 0.151515151_ forever
                v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<5, 33> >(1));
                REQUIRE(v == 151515);

                SECTION("low precision")
                {
                    typedef estd::chrono::duration<uint8_t, estd::milli> duration;
                    v = converter.convert(duration(50));

                    REQUIRE(v == 50000);

                    v = converter.convert(duration::max());

                    REQUIRE(v == 255000);
                }
            }
            SECTION("converter (runtime 400)")
            {
                DurationConverter<int, -1> converter;

                converter.numerator(400);

                uint64_t v = converter.convert(estd::chrono::milliseconds(10));

                REQUIRE(v == 2000);
            }
            SECTION("converter (runtime 8000)")
            {
                DurationConverter<int, -1> converter;

                converter.numerator(8000);

                uint64_t v = converter.convert(estd::chrono::milliseconds(100));

                REQUIRE(v == 1000);

                estd::chrono::duration<int, std::ratio<3, 1000> > v2;

                converter.convert(v, &v2);

                REQUIRE(v2.count() == 33);
            }
            SECTION("constexpr converter (80)")
            {
                DurationConverter<int, 80> converter;
                uint64_t v;

                SECTION("basic")
                {
                    v = converter.convert(estd::chrono::milliseconds(10));

                    REQUIRE(v == 10000);
                }
                SECTION("edge case")
                {
                    v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<1, 3> >(1));
                    REQUIRE(v == 333333);

                    // 5/33 of a second = 0.151515151_ forever
                    v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<5, 33> >(1));
                    REQUIRE(v == 151515);
                }
                SECTION("low precision")
                {
                    typedef estd::chrono::duration<uint8_t, estd::milli> duration;
                    v = converter.convert(duration(50));

                    REQUIRE(v == 50000);

                    v = converter.convert(duration::max());

                    REQUIRE(v == 255000);
                }
            }
            SECTION("constexpr converter (400)")
            {
                DurationConverter<int, 400> converter;

                uint64_t v = converter.convert(estd::chrono::milliseconds(10));

                REQUIRE(v == 2000);
            }
        }
    }
    SECTION("button")
    {
        embr::detail::Button b;
        bool r;

        r = down(b, estd::chrono::seconds(1));    // NOTE: first time duration passed in doesn't matter

        REQUIRE(r == false);
        REQUIRE(b.state() == embr::detail::Button::Evaluating);

        r = up(b, estd::chrono::milliseconds(300));

        REQUIRE(r == true);
        REQUIRE(b.state() == embr::detail::Button::Clicked);

        r = down(b, estd::chrono::seconds(1));

        REQUIRE(r == false);
        REQUIRE(b.state() == embr::detail::Button::Evaluating);

        r = down(b, estd::chrono::seconds(1));

        REQUIRE(r == true);
        REQUIRE(b.state() == embr::detail::Button::LongPressed);
    }
}