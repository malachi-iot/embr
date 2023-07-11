#include <catch2/catch.hpp>

#include <embr/internal/bitset.h>

using namespace estd;

TEST_CASE("bitset test", "[bitset]")
{
    bitset<17> b;
    bitset<5> b2{2};

    SECTION("verify init")
    {
        REQUIRE(b[0] == false);
        REQUIRE(b.test(0) == false);
        REQUIRE(b.none());
    }
    SECTION("set")
    {
        b.set(0);

        REQUIRE(b[0] == true);
        REQUIRE(b[1] == false);
        REQUIRE(b.any());

        b.reset(0);

        REQUIRE(b[0] == false);
        REQUIRE(b.none());
    }
    SECTION("flip")
    {
        b.flip(0);

        REQUIRE(b[0] == true);

        b.flip();

        REQUIRE(b[0] == false);
        REQUIRE(b[1] == true);

        b.flip(0);

        REQUIRE(b.all());
    }
    SECTION("to_string")
    {
        b2.set(4);

        SECTION("explicit")
        {
            estd::layer1::string<32> s;

            b2.to_string(s);

            REQUIRE(s == "10010");
        }
        SECTION("std style")
        {
            REQUIRE(b2.to_string() == "10010");
        }
    }
    SECTION("non standard")
    {
        b.set<1>();

        REQUIRE(b2.any());
        REQUIRE(b2.test<1>() == true);
        REQUIRE(b2.test<0>() == false);
    }
}
