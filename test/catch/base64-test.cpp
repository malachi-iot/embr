#include <catch2/catch_all.hpp>

#include <estd/sstream.h>

#include <embr/text/base64.h>

using namespace embr::text;

TEST_CASE("base64", "[text base64 streambuf]")
{
    using sstreambuf = estd::layer1::stringbuf<256>;

    SECTION("internals")
    {

    }
    SECTION("decode")
    {

    }
    SECTION("encode")
    {
        out_base64_streambuf<sstreambuf> out;

        // FIX: Linker error here on base64en
        //out.sputn("hi2u", 4);
    }
}
