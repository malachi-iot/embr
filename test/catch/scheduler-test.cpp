#include <catch.hpp>

#include <bitset>

#include <embr/scheduler.h>

// Not available because we test against C++11
//using namespace std::literals::chrono_literals;

struct Item
{
    int event_due;
    int* counter;
};

struct ItemTraits
{
    typedef Item value_type;
    typedef int time_point;

    static time_point get_time_point(const Item& item) { return item.event_due; }
    static bool process(Item& item, time_point)
    {
        if(item.counter != nullptr)
            ++(*item.counter);

        return false;
    }
};


struct Item2Traits
{
    typedef unsigned time_point;

    // NOTE: It's preferred to keep app state such as counter off in a pointed to struct
    // somewhere, since value_type here is copied around a lot during heap sorts.  That said,
    // it's not any kind of specific violation to keep app data in here
    struct value_type
    {
        int counter = 0;
        time_point wakeup;

        // DEBT: needed because vector doesn't currently know how to do unassigned by itself
        // that said, it is partially  spec behavior of vector:
        // https://stackoverflow.com/questions/29920394/vector-of-class-without-default-constructor
        // however ours deviates because we out of necessity pre allocate
        value_type() = default;

        value_type(time_point wakeup) : wakeup{wakeup} {}
        value_type(const value_type&) = default;

        /* although enabling all the following works, it hits me we prefer to lean heavily towards
         * copy-only because move operations imply greater short term overhead and we want this struct
         * as lightweight as possible
        value_type(value_type&&) = default;

        value_type& operator=(const value_type&) = default;
        value_type& operator=(value_type&&) = default; */
    };

    static time_point get_time_point(const value_type& v) { return v.wakeup; }

    static bool process(value_type& v, time_point)
    {
        v.wakeup += 10;
        ++v.counter;
        return true;
    }
};


struct Item3Traits
{
    typedef estd::chrono::steady_clock::time_point time_point;

    struct control_structure
    {
        typedef Item3Traits::time_point time_point;

        time_point t;

        virtual bool process(time_point current_time) = 0;
    };

    typedef control_structure* value_type;

    static time_point get_time_point(value_type v) { return v->t; }

    static bool process(value_type v, time_point t)
    {
        return v->process(t);
    }
};

struct Item3ControlStructure1 : Item3Traits::control_structure
{
    int counter = 0;

    virtual bool process(time_point current_time)
    {
        ++counter;
        // DEBT: Looks like estd::chrono doesn't have these overloads sorted yet
        t += std::chrono::seconds(10);
        return true;
    }
};

struct Item3ControlStructure2 : Item3Traits::control_structure
{
    int counter = 0;

    virtual bool process(time_point current_time)
    {
        ++counter;
        //t += std::chrono::seconds(5);
        return false;
    }
};


struct TraditionalTraitsBase
{
    typedef unsigned long time_point;

    struct control_structure
    {
        typedef bool (*handler_type)(control_structure* c, time_point current_time);

        time_point wake_time;
        handler_type handler;

        void* data;
    };
};

template <bool is_inline>
struct TraditionalTraits;

template <>
struct TraditionalTraits<true> : TraditionalTraitsBase
{
    typedef control_structure value_type;

    static time_point get_time_point(const value_type& v) { return v.wake_time; }

    static bool process(value_type& v, time_point current_time)
    {
        return v.handler(&v, current_time);
    }
};

template <>
struct TraditionalTraits<false> : TraditionalTraitsBase
{
    typedef control_structure* value_type;

    static time_point get_time_point(value_type v) { return v->wake_time; }

    static bool process(value_type v, time_point current_time)
    {
        return v->handler(v, current_time);
    }
};

bool traditional_handler(
    TraditionalTraitsBase::control_structure* c,
    unsigned long current_time)
{
    ++(*(int*)c->data);
    return false;
}

struct FunctorTraits
{
    typedef unsigned time_point;
    typedef estd::experimental::function_base<bool(time_point*, time_point)> function_type;

    template <class F>
    static estd::experimental::inline_function<F, bool(time_point*, time_point)> make_function(F&& f)
    {
        return estd::experimental::function<bool(time_point*, time_point)>::make_inline2(std::move(f));
    }

    struct control_structure
    {
        time_point wake;

        // DEBT: back to vector thing, function_base needs to be initialized with something
        // it doesn't have the full-function concept of an empty function
        //function_type func;

        function_type* func;

        /*
        control_structure(time_point wake, function_type::concept& c) :
            wake(wake),
            func(&c)
        {} */

        // DEBT: Have to do it this way because ::concept is protected still
        /*
        template <class TConcept>
        control_structure(time_point wake, TConcept c) :
            wake(wake),
            func(c)
        {} */

        control_structure(time_point wake, function_type* func) :
            wake(wake),
            func(func)
        {}

        // DEBT: See Item2Traits
        control_structure() = default;
    };

    typedef control_structure value_type;

    static time_point get_time_point(const value_type& v) { return v.wake; }

    static bool process(value_type& v, time_point current_time)
    {
        //return v.func();
        return (*(v.func))(&v.wake, current_time);
    }
};


TEST_CASE("scheduler test", "[scheduler]")
{
    SECTION("one-shot")
    {
        // doesn't have 'accessor', and maybe array isn't a good fit for priority_queue anyway
        //typedef estd::array<int, 4> container_type;
        typedef estd::layer1::vector<Item, 20> container_type;
        embr::internal::Scheduler<container_type, ItemTraits> scheduler;
        int counter = 0;

        scheduler.schedule(Item{5, &counter});
        scheduler.schedule(Item{6, &counter});
        scheduler.schedule(Item{10, &counter});
        scheduler.schedule(Item{11, &counter});

        auto top = scheduler.top();

        const Item& value = top.clock();

        REQUIRE(value.event_due == 5);

        top.cunlock();

        scheduler.process(10);

        REQUIRE(counter == 3);
        REQUIRE(scheduler.size() == 1);
    }
    SECTION("repeating")
    {
        embr::internal::layer1::Scheduler<Item2Traits, 5> scheduler;

        scheduler.schedule(5);
        scheduler.schedule(99); // should never reach this one

        for(Item2Traits::time_point i = 0; i < 50; i++)
        {
            scheduler.process(i);
        }

        // being that we continually reschedule, we'll always be at the top
        auto& v = scheduler.top().clock();

        // Should wake up 5 times at 5, 15, 25, 35 and 45
        REQUIRE(v.counter == 5);
    }
    SECTION("events (aux)")
    {
        int counter = 0;
        int _counter = 0;

        typedef estd::layer1::vector<Item, 20> container_type;
        auto o1 = embr::experimental::make_delegate_observer([&counter](
            const embr::internal::events::Scheduled<ItemTraits>& scheduled)
            {
                ++counter;
            });
        auto o2 = embr::experimental::make_delegate_observer([&counter](
            const embr::internal::events::Removed<ItemTraits>& removed)
            {
                --counter;
            });

        // NOTE: May be wanting a ref evaporator not a struct evaporator for scheduler
        auto s = embr::layer1::make_subject(o1, o2);
        typedef decltype(s) subject_type;
        embr::internal::Scheduler<container_type, ItemTraits, subject_type> scheduler(std::move(s));

        scheduler.schedule(Item{5});

        REQUIRE(counter == 1);

        scheduler.schedule(Item{7});

        REQUIRE(counter == 2);

        scheduler.process(6);

        // process removes one, so that will bump down our counter
        REQUIRE(counter == 1);
    }
    SECTION("virtual")
    {
        embr::internal::layer1::Scheduler<Item3Traits, 5> scheduler;

        Item3ControlStructure1 schedule1;
        Item3ControlStructure2 schedule2;

        scheduler.schedule(&schedule1);
        scheduler.schedule(&schedule2);

        estd::chrono::steady_clock::time_point now;
        estd::chrono::steady_clock::time_point end = now + std::chrono::seconds(60);

        for(; now < end; now += std::chrono::seconds(2))
        {
            scheduler.process(now);
        }

        REQUIRE(schedule1.counter == 6);
        REQUIRE(schedule2.counter == 1);
    }
    SECTION("traditional")
    {
        SECTION("inline")
        {
            typedef TraditionalTraits<true> traits_type;
            typedef traits_type::value_type value_type;
            int counter = 0;

            embr::internal::layer1::Scheduler<traits_type, 5> scheduler;

            scheduler.schedule(value_type{10, traditional_handler, &counter});
            scheduler.schedule(value_type{20, traditional_handler, &counter});

            scheduler.process(5);
            scheduler.process(10);
            scheduler.process(19);
            scheduler.process(21);

            REQUIRE(counter == 2);
        }
        SECTION("user allocated")
        {
            typedef TraditionalTraits<false> traits_type;
            typedef traits_type::value_type value_type;
            int counter = 0;

            embr::internal::layer1::Scheduler<traits_type, 5> scheduler;

            traits_type::control_structure
                scheduled1{10, traditional_handler, &counter},
                scheduled2{20, traditional_handler, &counter};

            scheduler.schedule(&scheduled1);
            scheduler.schedule(&scheduled2);

            scheduler.process(5);
            scheduler.process(10);
            scheduler.process(19);
            scheduler.process(21);

            REQUIRE(counter == 2);
        }
    }
    SECTION("experimental")
    {
        // Works well, but overly verbose on account of estd::experimental::function
        // indeed being in progress and experimental
        SECTION("estd::function style")
        {
            std::bitset<32> arrived;
            embr::internal::layer1::Scheduler<FunctorTraits, 5> scheduler;

            SECTION("trivial scheduling")
            {
                auto f_set_only = FunctorTraits::make_function([&arrived](unsigned* wake, unsigned current_time)
                {
                    arrived.set(*wake);
                    return false;
                });

                auto f_set_and_repeat = FunctorTraits::make_function([&arrived](unsigned* wake, unsigned current_time)
                {
                    arrived.set(*wake);
                    *wake = *wake + 2;
                    return true;
                });

                scheduler.schedule(11, &f_set_and_repeat);
                scheduler.schedule(3, &f_set_only);
                scheduler.schedule(9, &f_set_only);

                scheduler.process(0);
                scheduler.process(4);
                scheduler.process(9);
                scheduler.process(11);
                scheduler.process(20);

                REQUIRE(!arrived[0]);
                REQUIRE(arrived[3]);
                REQUIRE(!arrived[4]);
                REQUIRE(arrived[9]);
                REQUIRE(arrived[11]);
                REQUIRE(!arrived[12]);
                REQUIRE(arrived[13]);
                REQUIRE(arrived[15]);
                REQUIRE(arrived[17]);
                REQUIRE(!arrived[18]);
                REQUIRE(arrived[19]);
            }
            SECTION("overly smart scheduling")
            {
                auto _f = estd::experimental::function<bool(unsigned*, unsigned)>::make_inline(
                    [&arrived](unsigned* wake, unsigned current_time) {
                        arrived.set(*wake);
                        if (*wake < 11 || *wake > 20)
                            return false;
                        else
                        {
                            *wake = *wake + 2;
                            return true;
                        }
                    });
                estd::experimental::function_base<bool(unsigned*, unsigned)> f(&_f);

                scheduler.schedule(11, &f);
                scheduler.schedule(3, &f);
                scheduler.schedule(9, &f);

                scheduler.process(0);
                REQUIRE(!arrived[0]);
                scheduler.process(4);
                REQUIRE(arrived[3]);
                REQUIRE(!arrived[9]);
                scheduler.process(9);
                REQUIRE(arrived[9]);
                scheduler.process(11);
                REQUIRE(arrived[11]);
                scheduler.process(20);
                REQUIRE(!arrived[12]);
                REQUIRE(arrived[13]);
                REQUIRE(!arrived[14]);
                REQUIRE(arrived[15]);
                REQUIRE(!arrived[16]);
            }
        }
    }
}
