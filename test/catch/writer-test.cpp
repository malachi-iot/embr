#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-writer.h>

#include <estd/string.h>

TEST_CASE("writer test", "[writer]")
{
    SECTION("Static layer1 netbuf")
    {
        embr::mem::NetBufWriter<embr::mem::layer1::NetBuf<64>> writer;

        REQUIRE(writer.total_size() == 64);
    }
    SECTION("Static layer2 netbuf")
    {
        embr::mem::layer2::NetBuf<128> netbuf;
        embr::mem::NetBufWriter<decltype(netbuf)&> writer(netbuf);

        // NOTE: it's 0 because the vector has not been added to yet
        // a bit peculiar, but perhaps after getting use to it it'll
        // sit right
        REQUIRE(writer.buffer().size() == 0);

        SECTION("basic write")
        {
            REQUIRE(writer.next(100));
            REQUIRE(writer.total_size() == 100);
            estd::mutable_buffer b = writer.buffer();

            estd::layer3::string s((char*)b.data(), 0, b.size());
            //writer.buffer();

            s += "Hi2u";

            REQUIRE((char)netbuf[0] == 'H');
            REQUIRE((char)netbuf[1] == 'i');
            REQUIRE((char)netbuf[2] == '2');
            REQUIRE((char)netbuf[3] == 'u');
        }
    }
}
