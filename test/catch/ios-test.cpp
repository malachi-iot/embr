#include <catch.hpp>

#include <embr/streambuf.h>
#include <embr/netbuf-static.h>
#include <estd/string.h>

using namespace embr;

TEST_CASE("iostreams", "[ios]")
{
    SECTION("basic output netbuf+streambuf")
    {
        mem::layer1::NetBuf<32> nb;
        mem::impl::out_netbuf_streambuf<char, mem::layer1::NetBuf<32>& > sb(nb);

        sb.xsputn("hi2u", 5); // cheat and include null termination also

        // TODO: make typedef layer1::string include provision to specify null termination
        // FIX: estd::layer2::string<> doesn't compile
        //estd::layer2::string<> s((char*)nb.data());
        char* helper = reinterpret_cast<char*>(nb.data());

        estd::layer2::const_string s(helper);

        REQUIRE(s == "hi2u");
    }
}
