#include <catch2/catch_all.hpp>

#include <embr/json/decoder.h>
#include <embr/json/encoder.hpp>

#include <estd/sstream.h>
#include <estd/string_view.h>

using namespace embr::json;

#include "test-data.h"

using bc = embr::internal::breadcrumb;

enum nav_ids
{
    id_top,
    id_lvl1_0,
    id_lvl1_1,
    id_lvl2,
    id_lvl2_0,
    id_lvl2_1,
};

static constexpr bc nav[]
{
    { "top",    id_top },
    { "lvl1.0", id_lvl1_0,  id_top },
    { "lvl1.1", id_lvl1_1,  id_top },
    { "lvl2",   id_lvl2,    id_top },
    { "lvl2.0", id_lvl2_0,  id_top },
    { "lvl2.1", id_lvl2_1,  id_top },
    { nullptr }
};

static const char* json1 =
    R"=({"hi2u"})=";

static const char* json2 =
    R"=({"hi2u": true})=";

static const char* json3 =
    R"=({"hi2u": 123.45})=";

static const char* json_array =
    R"=({"hi2u": [1, 2, 3, 4, "hi", { "el1": 5 } ] })=";

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
    SECTION("decoder v1: bits")
    {
        using decoder_type = internal::decoder;
        using state_type = const internal::decoder_state&;
        decoder_type decoder;
        unsigned counter = 0;

        using iss = estd::layer2::basic_istringstream<const char>;

        SECTION("number")
        {
            iss in("123.4");

            decoder.decode(in, [&](state_type state, const decoder_type::descriptor& d)
            {
                if(state.item() == decoder_type::NUMBER)
                {
                    REQUIRE(d.number == 123.4);
                    ++counter;
                }
            });

            REQUIRE(counter == 1);
        }
        SECTION("literal")
        {
            iss in("null");

            decoder.decode(in, [&](state_type state, const decoder_type::descriptor& d)
            {
                if (state.item () == decoder_type::LITERAL)
                {
                    REQUIRE(d.literal == internal::ID_NULL);
                    ++counter;
                }
            });

            REQUIRE(counter == 1);
        }
    }
    SECTION("decoder v1: holistic")
    {
        using decoder_type = internal::decoder;
        using istream_type = estd::layer2::basic_istringstream<const char>;

        decoder_type decoder;
        int counter = 0;
        char temp[32];

        SECTION("minimal")
        {
            istream_type in(json1);

            decoder.decode(in, [&](const internal::decoder_state& d, const decoder_type::descriptor& i)
            {
                if(d.state() != decoder_type::TOKEN_END) return;

                if(d.item() == decoder_type::NAME)
                {
                    i.str(in, temp);

                    REQUIRE(estd::layer2::const_string(temp) == "hi2u");
                    ++counter;

                    REQUIRE(i.str(in) == "hi2u");
                }
            });
        }
        SECTION("number")
        {
            istream_type in(json3);

            decoder.decode(in, [&](
                const internal::decoder_state& d,
                const decoder_type::descriptor& i)
            {
                if(d.state() != decoder_type::TOKEN_END) return;

                if(d.item() == decoder_type::NAME)
                {
                    REQUIRE(i.str(in) == "hi2u");
                    ++counter;
                }
                else if(d.item() == decoder_type::NUMBER)
                {
                    REQUIRE_THAT(i.number, Catch::Matchers::WithinAbsMatcher(123.45, 0.00001));
                    ++counter;
                }
            });

            REQUIRE(counter == 2);
        }
        SECTION("array")
        {
            istream_type in(json_array);
            int array_count = 0;

            decoder.decode(in, [&](
                const internal::decoder_state& d,
                const decoder_type::descriptor& i)
            {
                if(d.has_parent() && d.parent()->item() == decoder_type::ARRAY)
                {
                    if(d.state() != decoder_type::TOKEN_END)
                    {
                        // DEBT: Clumsiness because TOKEN_START means char by char for strings,
                        // but announcement of upcoming OBJECT
                        if(d.item() == decoder_type::OBJECT)
                        {
                            ++array_count;

                            //++counter;
                        }
                        return;
                    }

                    ++array_count;

                    if(d.item() == decoder_type::NUMBER)
                    {
                        counter += i.number;
                    }
                    else if(d.item() == decoder_type::STRING)
                    {
                        REQUIRE(i.str(in) == "hi");
                        ++counter;
                    }
                }
            });

            REQUIRE(array_count == 6);
            REQUIRE(counter == 1 + 2 + 3 + 4 + 1);
        }
    }
    // DEBT: Test belongs elsewhere
    SECTION("breadcrumbs")
    {
        SECTION("plain search")
        {
            const bc* found = embr::internal::search(nav, "top");

            REQUIRE(found->id == id_top);

            found = embr::internal::search(found + 1, "top");

            // Early days for breadcrumb search.  This flavor searches until we leave
            // a parent domain, which starts as -1
            REQUIRE(found->parent != -1);
        }
        SECTION("stateful")
        {
            using searcher = embr::internal::searcher;

            SECTION("lvl1.1")
            {
                const embr::internal::breadcrumb* r = search2(nav + 1, "lvl1.1");

                REQUIRE(r);
                REQUIRE(r->id == id_lvl1_1);
            }
            SECTION("lvl2.0")
            {
                const embr::internal::breadcrumb* r = search2(nav + 1, "lvl2.0");

                REQUIRE(r);
                REQUIRE(r->id == id_lvl2_0);
            }
        }
    }
}
