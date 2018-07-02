#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-writer.h>

#include <estd/string.h>

TEST_CASE("writer test", "[writer]")
{
    SECTION("Static netbuf")
    {
        embr::mem::layer2::NetBuf<128> netbuf;
        embr::mem::NetBufWriter<decltype(netbuf)&> writer(netbuf);

        // NOTE: it's 0 because the vector has not been added to yet
        // a bit peculiar, but perhaps after getting use to it it'll
        // sit right
        REQUIRE(writer.buffer().size() == 0);

        SECTION("basic write")
        {
        }
    }
}
