#include <catch2/catch_all.hpp>

#include <estd/sstream.h>

#include <embr/text/base64.h>

using namespace embr::text;

TEST_CASE("base64", "[text base64 streambuf]")
{
    using sstreambuf = estd::layer1::stringbuf<256>;
    using string_type = sstreambuf::string_type;

    SECTION("internals")
    {

    }
    SECTION("decode")
    {

    }
    SECTION("encode")
    {
        // Testing helped by https://emn178.github.io/online-tools/base64_encode.html
        out_base64_streambuf<sstreambuf> out;
        auto& w = out.wrapped();
        const string_type& str = out.wrapped().str();

        out.sputc('h');
        out.finalize(true);

        REQUIRE(str == "aA==");

        w.clear();

        out.sputc('h');
        out.sputc('i');
        out.finalize(true);

        REQUIRE(str == "aGk=");

        w.clear();

        out.sputn("hi!", 3);
        out.finalize(true);

        REQUIRE(str == "aGkh");

        w.clear();

        out.sputn("{\"key\"}", 6);
        out.finalize(true);

        // FIX: Problem manifests here
        //REQUIRE(str == "eyJrZXkifQ==");

        out.sputn("{\"key\": 00}", 10);
        out.finalize(true);

        //REQUIRE(str == "eyJrZXkiOjAwfQ==");
    }
}
