#include <catch.hpp>

#include <embr/word.h>

using namespace embr;

TEST_CASE("word type test", "[word]")
{
    SECTION("enum_mask")
    {
        SECTION("low level")
        {
            typedef embr::internal::enum_mask<word_strictness, word_strictness::masking> h;

            REQUIRE(h::all<word_strictness::masking>());
            REQUIRE(h::any<word_strictness::masking, word_strictness::narrowing>());
            REQUIRE(!h::all<h::e::masking, h::e::narrowing>());

            auto v = h::or_all<
                word_strictness::masking, word_strictness::narrowing, word_strictness::arithmetic>::value;
        }
        SECTION("high level")
        {
        }
    }
    SECTION("8 bit")
    {
        embr::word<1> v{true};
        typedef estd::numeric_limits<decltype(v)> limits;
    }
    SECTION("16 bit")
    {
        embr::word<10> v{3};
        typedef estd::numeric_limits<decltype(v)> limits;

        v = v + 3;

        REQUIRE(v == 6);
        REQUIRE(v.width() == 10);
        REQUIRE(limits::min() == 0);
        REQUIRE(limits::max() == 1023);

        v += 10;

        REQUIRE(v == 16);
    }
    SECTION("narrowing")
    {
        embr::word<36> v{3};

        /*
         * Compiler will not allow this (by our design), unsupported narrowing */
        //auto v_short = estd::to_integer<short>(v);

        //REQUIRE(v_short == 3);

        auto v_long = estd::to_integer<long>(v);

        REQUIRE(v_long == 3);

        // We disallow this narrowing also
        //embr::word<10> v2{v};

        auto v_short1 = embr::narrow_cast<word<10, true> >(v);

        REQUIRE(v_short1 == 3);
    }
}