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
    static bool process(Item& item)
    {
        ++(*item.counter);

        return false;
    }
};


struct Item2Traits
{
    typedef unsigned time_point;

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
        //value_type(value_type&&) = default;
    };

    static time_point get_time_point(const value_type& v) { return v.wakeup; }

    static bool process(value_type& v)
    {
        v.wakeup += 10;
        ++v.counter;
        return true;
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
        // doesn't have 'accessor', and maybe array isn't a good fit for priority_queue anyway
        //typedef estd::array<int, 4> container_type;
        typedef estd::layer1::vector<Item2Traits::value_type, 20> container_type;
        embr::internal::Scheduler<container_type, Item2Traits> scheduler;

        scheduler.schedule(5);

        for(Item2Traits::time_point i = 0; i < 50; i++)
        {
            scheduler.process(i);
        }

        // being that we continually reschedule and this is the only item being scheduled,
        // we'll always be at the top
        auto& v = scheduler.top().clock();

        // Should wake up 5 times at 5, 15, 25, 35 and 45
        REQUIRE(v.counter == 5);
    }
}
