#include <catch2/catch_all.hpp>

#include <embr/storage/objlist.h>

TEST_CASE("Object list, Object stack", "[objlist]")
{
    embr::layer1::objlist<256> objlist;

    objlist.alloc(16);

    embr::detail::v1::objlist_element<2> elem(0, -1);

    REQUIRE(elem.next_diff() == -4);
}
