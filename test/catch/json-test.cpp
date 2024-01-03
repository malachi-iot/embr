#include <catch2/catch.hpp>

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
    estd::detail::basic_ostream<estd::layer1::basic_out_stringbuf<char, 128> > out;
    auto& str = out.rdbuf()->str();

    SECTION("v1")
    {
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
                make_fluent(e, out)

                ("user")
                    ("name", "Fred")
                ();
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
}
