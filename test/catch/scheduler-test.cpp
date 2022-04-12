#include <catch.hpp>

#include <embr/scheduler.h>

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

    struct value_type
    {
        time_point t;

        virtual bool process(time_point current_time)
        {
            return false;
        }
    };

    static time_point get_time_point(const value_type& v) { return v.t; }

    static bool process(value_type& v, time_point t) { return v.process(t); }
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
    SECTION("events")
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
}
