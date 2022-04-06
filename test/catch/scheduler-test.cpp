#include <catch.hpp>

#include <embr/scheduler.h>

TEST_CASE("scheduler test", "[scheduler]")
{
    SECTION("a")
    {
        // doesn't have 'accessor', and maybe array isn't a good fit for priority_queue anyway
        //typedef estd::array<int, 4> container_type;
        typedef estd::layer1::vector<int, 20> container_type;
        embr::internal::Scheduler<container_type> scheduler;

        // this crashes, presumably due to a glitch in Compare handling
        //scheduler.schedule(5);
        //scheduler.schedule(6);
    }
}
