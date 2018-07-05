#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-reader.h>

#include <estd/string.h>
#include <estd/string_view.h>

TEST_CASE("reader test", "[reader]")
{
    SECTION("Static netbuf")
    {
        embr::mem::layer2::NetBuf<128> netbuf = { 'H', 'e', 'l', 'l', 'o' };
        embr::mem::NetBufReader<decltype(netbuf)&> reader(netbuf);

        SECTION("basic read")
        {
            estd::layer3::const_string s = reader.buffer();

            REQUIRE(s == "Hello");

            reader.advance(4);

            REQUIRE(reader.buffer()[0] == 'o');
        }
        SECTION("byte >> operator")
        {
            uint8_t value;

            reader >> value;

            REQUIRE(value == netbuf.data()[0]);
        }
    }
}
