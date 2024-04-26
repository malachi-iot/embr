#include <catch2/catch.hpp>

#include <embr/exp/thunk.h>

TEST_CASE("thunk")
{
    SECTION("pt1")
    {
        embr::experimental::Thunk t;
        int val = 0;

        t.enqueue([&](void*)
        {
            ++val;
        });
        /*
        t.enqueue([&](void*)
        {
            ++val;
        }); */
        t.invoke();
        t.invoke();

        REQUIRE(val == 1);
    }
}