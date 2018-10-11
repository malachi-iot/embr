#include <catch.hpp>

#include <embr/datapump.hpp>
#include <embr/exp/datapump-v2.h>
#include "datapump-test.h"

// specifically for v2 experimental datapump/dataport
struct FakeTransport
{
    typedef void* pbuf_type;
    typedef int addr_type;


};

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

        SECTION("raw datapump")
        {
            Datapump2<void*, int> datapump;

            datapump.enqueue_from_transport((void*) "test", 0);

            REQUIRE(datapump.from_transport_ready());

            auto item = datapump.dequeue_from_transport();

            REQUIRE(item->addr == 0);

            datapump.deallocate(item);
        }
        SECTION("dataport")
        {
            Dataport2<Datapump2<void*, int> > dataport;

            dataport.process();
        }
    }
}
