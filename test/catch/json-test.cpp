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

    SECTION("v1")
    {
        v1::encoder<single_quoted> e;

        SECTION("encoder")
        {
            e.begin(out);
            e.end(out);
        }
        SECTION("fluent")
        {
            auto j = make_fluent(e, out);

            SECTION("user")
            {
                j.begin()

                ("user")
                    ("age", 30)
                    ("name", "Fred")
                --;

                j.end();

                REQUIRE(out.rdbuf()->str() == test::json_user);
            }
            SECTION("array")
            {
                j.begin()

                ["prefs"] (1, 2, 3, "hi2u");

                j.end();

                REQUIRE(out.rdbuf()->str() == test::json_prefs);
            }
        }
    }
}
