#include <catch2/catch_all.hpp>

#include <random>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"

using namespace embr::mem;

TEST_CASE("gc mem v1 low level tests", "[memory][gc][ll]")
{
    constexpr unsigned pool_size = 2048;

    using page = detail::v1::page<uint16_t>;
    using handles_traits = detail::v1::handles_traits<page[10]>;
    using handles_type = detail::v1::handles<handles_traits>;
    using pool_traits = detail::v1::pool_traits<char[pool_size]>;
    using pool_type = detail::v1::pool<pool_traits>;
    using ops_type = pool_type::ops<handles_traits>;
    using block = detail::v1::block;
    using bundle = ops_type::bundle;
    using pos_type = page::unit_type;

    pool_type pool;
    handles_type handles;

    ops_type ops = ops_type{pool, handles};

    // DEBT: Still having to do this
    ops.reset();

    bundle b0 = ops.alloc(pos_type(8), block::Trivial);
    bundle b1 = ops.get_bundle(1);

    REQUIRE(b0.handle == 0);
    REQUIRE(b0.allocated());
    REQUIRE(b1.allocated() == false);

    SECTION("virtual_swap")
    {
        SECTION("adjacent forward")
        {
            //ops.virtual_swap(b0, b1);
        }
        SECTION("adjacent reverse")
        {
            //ops.virtual_swap(b1, b0);
        }
        SECTION("non-adjacent forward")
        {

        }
        SECTION("non-adjacent reverse")
        {

        }
    }
}
