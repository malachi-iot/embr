#include <catch.hpp>

#include <embr/datapump.hpp>
#include <embr/exp/datapump-v2.h>
#include "datapump-test.h"

TEST_CASE("datapump")
{
    synthetic_netbuf_type nb;
    synthetic_datapump dp;

    SECTION("A")
    {

    }
    SECTION("v2 (experimental)")
    {
        using namespace embr::experimental;

        Datapump2<void*, int> datapump;

        datapump.enqueue_from_transport((void*)"test", 0);

        REQUIRE(datapump.from_transport_ready());

        auto item = datapump.dequeue_from_transport();

        REQUIRE(item->addr == 0);

        datapump.deallocate(item);
    }
}
