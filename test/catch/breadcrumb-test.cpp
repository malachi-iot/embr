#include <catch2/catch_all.hpp>

#include <cmrc/cmrc.hpp>

#include <embr/internal/breadcrumb.h>

#include <estd/sstream.h>
#include <estd/string_view.h>

//#include "test-data.h"

using bc = embr::internal::breadcrumb;

enum nav_ids
{
    id_top,
    id_lvl1_0,
    id_lvl1_1,
    id_lvl1_1_1,
    id_lvl1_1_2,
    id_lvl1_1_2_1,
    id_lvl2,
    id_lvl2_0,
    id_lvl2_0_0,
    id_lvl2_1,
    id_side1,
};



static constexpr bc nav[]
{
    { "top",        id_top },
    { "lvl1.0",     id_lvl1_0,      id_top },
    { "lvl1.1",     id_lvl1_1,      id_top },
    { "lvl1.1.1",   id_lvl1_1_1,    id_lvl1_1 },
    { "lvl1.1.2",   id_lvl1_1_2,    id_lvl1_1 },
    { "lvl1.1.2.1", id_lvl1_1_2_1,  id_lvl1_1_2 },
    { "lvl2",       id_lvl2,        id_top },
    { "lvl2.0",     id_lvl2_0,      id_top },
    { "lvl2.0.0",   id_lvl2_0_0,    id_lvl2_0 },
    { "lvl2.1",     id_lvl2_1,      id_top },
    { "side1",      id_side1 },
    { nullptr }
};


TEST_CASE("breadcrumb tests", "[breadcrumb]")
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
        SECTION("lvl1.1.2")
        {
            const embr::internal::breadcrumb* r = search2(nav + 3, "lvl1.1.2");

            REQUIRE(r);
            REQUIRE(r->id == id_lvl1_1_2);
        }

        SECTION("lvl2.0")
        {
            const embr::internal::breadcrumb* r = search2(nav + 1, "lvl2.0");

            REQUIRE(r);
            REQUIRE(r->id == id_lvl2_0);
        }
    }
}
