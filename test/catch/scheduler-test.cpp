#include <catch.hpp>

#include <embr/scheduler.h>

struct Item
{
    int event_due;
    int* counter;
};

struct ItemTraits
{
    typedef int time_point;

    static time_point get_time_point(const Item& item) { return item.event_due; }
    static void process(Item& item)
    {
        ++(*item.counter);
    }
};

TEST_CASE("scheduler test", "[scheduler]")
{
    SECTION("a")
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
}
