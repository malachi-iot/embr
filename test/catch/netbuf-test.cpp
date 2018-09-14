#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>

using namespace embr;

TEST_CASE("netbuf")
{
    SECTION("static")
    {
        SECTION("layer1")
        {
            mem::layer1::NetBuf<64> nb;
        }
        // layer2 netbuf can expand/shrink (in a linear fashion, not chained)
        SECTION("layer2")
        {
            mem::layer2::NetBuf<64> nb;

            REQUIRE(nb.size() == 0);

            nb.expand(32, true);

            REQUIRE(nb.size() == 32);

            nb.shrink_experimental(16);

            REQUIRE(nb.size() == 16);
        }
    }
    SECTION("dynamic")
    {
        mem::experimental::NetBufDynamic<> nb;

        REQUIRE(nb.size() == 0);

        nb.expand(512, true);

        REQUIRE(nb.size() == 512);

        nb.shrink_experimental(64);
    }
}
