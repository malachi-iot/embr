#include <catch2/catch_all.hpp>

#include <cmrc/cmrc.hpp>

#include <embr/internal/breadcrumb.h>

#include <estd/sstream.h>
#include <estd/string_view.h>

//#include "test-data.h"

using bc = embr::internal::breadcrumb;

enum nav_ids
{
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


// NOTE: In real life, you'll not explicitly state the entire level in your string.  This is
// done here just for clarity of testing
static constexpr bc nav[]
{
    { "lvl1.0",     id_lvl1_0 },
    { "lvl1.1",     id_lvl1_1 },
    { "lvl1.1.1",   id_lvl1_1_1,    id_lvl1_1 },
    { "lvl1.1.2",   id_lvl1_1_2,    id_lvl1_1 },
    { "lvl1.1.2.1", id_lvl1_1_2_1,  id_lvl1_1_2 },
    { "lvl2",       id_lvl2 },
    { "lvl2.0",     id_lvl2_0 },
    { "lvl2.0.0",   id_lvl2_0_0,    id_lvl2_0 },
    { "lvl2.1",     id_lvl2_1 },
    { "side1",      id_side1 },
    bc::null()
};


TEST_CASE("breadcrumb tests", "[breadcrumb]")
{
    SECTION("plain search")
    {
        const bc* found = search_siblings(nav, "lvl1.0");

        REQUIRE(found->id == id_lvl1_0);

        found = embr::internal::search_siblings(next_sibling(found), "lvl1.0");

        REQUIRE(found == nullptr);

        found = embr::internal::search_siblings(nav + 1, "lvl1.1");

        REQUIRE(found->parent == -1);
        REQUIRE(found == nav + 1);

        // Paradigm is such that if children exist, they are the very next item after
        // the parent
        REQUIRE(has_children(found));
        found = first_child(found);

        REQUIRE(found);
        REQUIRE(found->parent == id_lvl1_1);
        found = embr::internal::search_siblings(found, "lvl1.1.2");

        REQUIRE(found->id == id_lvl1_1_2);
    }
    // Stateful mode is interesting if you're facing partial buffers, like LwIP or maybe CoAP appears over
    // a serial port
    SECTION("stateful")
    {
        using searcher = embr::breadcrumb::searcher;

        // DEBT: search2 is crude and seems I am only using it as a parity mechanism to
        // make sure stateful mode works as well as non-stateful.  In real life, I expect
        // one will directly want to use breadcrumb_searcher
        SECTION("lvl1.1")
        {
            const embr::internal::breadcrumb* r = search2(nav + 1, "lvl1.1");

            REQUIRE(r);
            REQUIRE(r->id == id_lvl1_1);
        }
        SECTION("lvl1.1.2")
        {
            SECTION("happy path")
            {
                const embr::internal::breadcrumb* r = search2(nav + 3, "lvl1.1.2");

                REQUIRE(r);
                REQUIRE(r->id == id_lvl1_1_2);
            }
            SECTION("wrong parent")
            {
                const embr::internal::breadcrumb* r = search2(nav + 1, "lvl1.1.2");

                REQUIRE(r == nullptr);
                //REQUIRE(r->id == id_lvl1_1_2);
            }
        }
        SECTION("lvl2.0")
        {
            // FIX: nav + 1 does not work, but should - so a bug is lurking in here somewhere
            const embr::internal::breadcrumb* r = search2(nav, "lvl2.0");

            REQUIRE(r);
            REQUIRE(r->id == id_lvl2_0);
        }
    }
    SECTION("misc")
    {
        constexpr bc bc1{"hi", 0, -1};

        REQUIRE(bc1 != bc::null());
    }
}
