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
        // doesn't have 'accessor', and maybe array isn't a good fit for priority_queue anyway
        //typedef estd::array<int, 4> container_type;
        typedef estd::layer1::vector<Item2Traits::value_type, 20> container_type;
        embr::internal::Scheduler<container_type, Item2Traits> scheduler;

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
        typedef estd::layer1::vector<Item3Traits::value_type, 20> container_type;
        embr::internal::Scheduler<container_type, Item3Traits> scheduler;

    }
}
