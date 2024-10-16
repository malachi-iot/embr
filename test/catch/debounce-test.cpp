#include <catch2/catch_all.hpp>

#include <embr/detail/debounce.hpp>
#include <embr/detail/button.hpp>

#include <embr/internal/debounce/ultimate.h>

#include <embr/exp/filter.h>

using namespace embr::detail;

namespace embr { namespace experimental {


//typedef fake_lpf<estd::chrono::milliseconds, 10> fake_lpf_ms;
typedef filter_wrapper<estd::chrono::milliseconds> fake_lpf_ms;


struct FancyPress
{
    // in ms
    typedef unsigned time_point;

    static time_point click_interval()
    {
        return 500;
    }

    time_point state_changed_timestamp_ = 0;

    enum States
    {
        UNDEFINED,
        SINGLE_PRESSED,
        SINGLE_RELEASED,
        SINGLE_CLICKED,
        DOUBLE_PRESSED,
        DOUBLE_CLICKED, // press and release twice
        TRIPLE_PRESSED,
        LONG_PRESSED,
        LONG_RELEASED
    };

    States state_ = UNDEFINED;

    void state(States s, time_point current)
    {
        state_ = s;
        state_changed_timestamp_ = current;
    }

    void eval_pressed(time_point current)
    {
        switch(state_)
        {
            case UNDEFINED:
                state(SINGLE_PRESSED, current);
                break;

            case SINGLE_PRESSED:
            case DOUBLE_PRESSED:
            case TRIPLE_PRESSED:
                // Undefined behavior to get another press even while
                // already evaluating a pressed event - that might change
                // if/when we enable the periodic emit of pressed messages
                break;

            default: break;
        }
    }

    void eval_released(time_point current)
    {
        switch(state_)
        {
            case UNDEFINED:
                break;

            default: break;
        }
    }

    void eval(embr::debounce::v1::States s, time_point current)
    {
        switch(s)
        {
            case embr::debounce::v1::States::On:
                eval_pressed(current);
                break;

            case embr::debounce::v1::States::Off:
                eval_released(current);
                break;

            default: break;
        }
    }
};


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
    SECTION("ultimate")
    {
        using States = embr::debounce::v1::States;
        SECTION("8-bit")
        {
            embr::debounce::ultimate::History<uint8_t> b;

            b.update(1);
            REQUIRE(eval(b) == States::Undefined);

            b.update(1);
            REQUIRE(eval(b) == States::Undefined);
            b.update(1);
            REQUIRE(eval(b) == States::On);

            b.update(0);
            REQUIRE(eval(b) == States::Undefined);
        }
        SECTION("16-bit")
        {
            embr::debounce::ultimate::History<uint16_t> b;

            b.update(1);
            REQUIRE(b.eval_on() == false);

            b.update(1);
            b.update(1);
            b.update(1);
            b.update(1);
            REQUIRE(b.eval_on() == false);
            b.update(1);
            REQUIRE(b.eval_on() == true);

            b.update(0);
            REQUIRE(b.eval_on() == false);
        }
        SECTION("tracker")
        {
            embr::debounce::ultimate::Tracker<uint8_t> t;

            REQUIRE(t.eval(1) == false);
            REQUIRE(t.eval(1) == false);
            REQUIRE(t.state() == States::Undefined);
            bool v = t.eval(1);
            REQUIRE(v == true);
            REQUIRE(t.state() == States::On);
        }
    }
}
