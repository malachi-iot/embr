#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-reader.h>

#include <estd/string.h>

using namespace embr::mem;

TEST_CASE("basics", "[basic]")
{
    SECTION("Size sanity checks")
    {
        typedef internal::NetBufWrapper<layer1::NetBuf<16>> type;

        REQUIRE(sizeof(type) == 16 + sizeof(type::size_type));
    }
}