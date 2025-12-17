#include <catch2/catch_all.hpp>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>

using namespace embr::mem;

TEST_CASE("gc mem v1 tests", "[memory][gc]")
{
    SECTION("detail")
    {
        SECTION("handles: layer1")
        {
            using page = detail::v1::page<uint16_t, estd::ratio<1>>;
            using unit_type = page::unit_type;
            using type = detail::v1::handles<detail::v1::handles_traits<page[20]>>;

            static_assert(sizeof(type) == 20 * 2);

            type handles;

            handles.alloc(
                [](page& v)
                {
                    v.pos(unit_type(0));
                });
        }
        SECTION("handles: layer2")
        {
            using page = detail::v1::page<uint16_t, estd::ratio<1>>;
            using unit_type = page::unit_type;
            using type = detail::v1::handles<detail::v1::handles_traits<estd::span<page, 20>>>;

        }
        SECTION("pool: layer1")
        {

        }
    }
}
