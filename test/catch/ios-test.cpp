#include <catch.hpp>

#include <embr/streambuf.h>
#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>

#include <estd/string.h>
#include <estd/ostream.h>

using namespace embr;

TEST_CASE("iostreams", "[ios]")
{
    mem::layer1::NetBuf<32> nb;
    SECTION("basic output netbuf+streambuf impl")
    {
        mem::impl::out_netbuf_streambuf<char, mem::layer1::NetBuf<32>& > sb(nb);

        sb.xsputn("hi2u", 5); // cheat and include null termination also

        // TODO: make typedef layer1::string include provision to specify null termination
        // FIX: estd::layer2::string<> doesn't compile
        //estd::layer2::string<> s((char*)nb.data());
        char* helper = reinterpret_cast<char*>(nb.data());

        estd::layer2::const_string s(helper);

        REQUIRE(s == "hi2u");
    }
    SECTION("proper netbuf_streambuf type (not impl) + ostream")
    {
        using namespace estd::internal;

        mem::netbuf_streambuf<char, mem::layer1::NetBuf<32>& > sb(nb);
        estd::internal::basic_ostream<decltype(sb)&> out(sb);

        out << 'a';

        REQUIRE(nb.data()[0] == 'a');

        // NOTE: Odd that this doesn't work.  no usings seem to clear it up either
        out << " nice day";

        REQUIRE(memcmp(nb.data(), "a nice day", 10)== 0);

        int sz = sizeof(sb);

        SECTION("Direct pointer access")
        {
            REQUIRE(sb.pbase() == (char*)nb.data());
        }
    }
    SECTION("dynamic")
    {
        mem::experimental::NetBufDynamic<> nb2;

        /*
        // FIX: this breaks, something about intrusive list
        nb2.expand(100, false);

        SECTION("streambuf")
        {
            mem::netbuf_streambuf<char, decltype (nb2)&> sb(nb2);

            sb.sputc('a');
        }
        SECTION("ostream")
        {

        } */
    }
}
