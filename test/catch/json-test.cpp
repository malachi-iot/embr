#include <catch2/catch_all.hpp>

#include <embr/json/decoder.h>
#include <embr/json/encoder.hpp>

#include <estd/sstream.h>

using namespace embr::json;

#include "test-data.h"

struct single_quoted : v1::options::lean
{
    static constexpr bool use_doublequotes() { return false; }

};

TEST_CASE("json tests", "[json]")
{
    SECTION("encoder v1")
    {
        estd::layer1::ostringstream<128> out;
        const estd::layer1::string<128>& str = out.rdbuf()->str();

        v1::encoder<single_quoted> e;

        SECTION("encoder")
        {
            e.begin(out);
            e.begin(out, "user");
            e.add(out, "age", 30);
            e.add(out, "name", "Fred");
            e.end(out);
            e.end(out);

            REQUIRE(str == test::json_user);
        }
        SECTION("fluent")
        {
            auto j = make_fluent(e, out);

            SECTION("minimal")
            {
                make_fluent(out)

                ("u")
                    ("a", 30)
                ();

                REQUIRE(str == "\"u\":{\"a\":30}");
            }
            SECTION("user")
            {
                REQUIRE(e.level() == 0);

                j.begin()

                ("user")
                    ("age", 30)
                    ("name", "Fred")
                ();

                REQUIRE(e.level() == 1);

                j.end();

                REQUIRE(e.level() == 0);

                REQUIRE(str == test::json_user);
            }
            SECTION("array")
            {
                j.begin()

                ["prefs"] (1, 2, 3, "hi2u");

                j.end();

                REQUIRE(str == test::json_prefs);
            }
            SECTION("int")
            {
                j("str", 10);

                REQUIRE(str == "'str':10");
            }
        }
    }
    SECTION("decoder v1")
    {
        internal::decoder decoder;

        // DEBT: Both correct and clumsy requiring const here
        estd::layer2::basic_istringstream<const char> in("");

        decoder.decode(in, [] {});
    }
}
