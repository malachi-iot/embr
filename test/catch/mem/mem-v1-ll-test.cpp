#include <catch2/catch_all.hpp>

#include <sstream>

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
    pos_type p0 = b0.pos();
    pos_type p1 = b1.pos();
    std::ostringstream out;

    REQUIRE(b0.handle == 0);
    REQUIRE(b0.allocated());
    REQUIRE(b1.allocated() == false);

    SECTION("virtual_swap")
    {
        SECTION("adjacent forward")
        {
            ops.virtual_swap(b0, b1);

            ops.dump(out << "\n");
            CAPTURE(out.str());
            REQUIRE(ops.invariant());

            REQUIRE(b0.pos() == p1);
            REQUIRE(b1.pos() == p0);
        }
        SECTION("adjacent reverse")
        {
            ops.virtual_swap(b1, b0);

            ops.dump(out << "\n");
            CAPTURE(out.str());
            REQUIRE(ops.invariant());

            REQUIRE(b0.pos() == p1);
            REQUIRE(b1.pos() == p0);
        }
        SECTION("non-adjacent")
        {
            b1 = ops.alloc(pos_type(8), block::Trivial);

            REQUIRE(b1.handle == 1);

            bundle b2 = ops.get_bundle(2);
            REQUIRE(b2.allocated() == false);
            REQUIRE(b2.has_next() == false);
            pos_type p2 = b2.pos();
            bool reversed = false;

            SECTION("forward")
            {
                ops.virtual_swap(b0, b2);
            }
            SECTION("reverse")
            {
                reversed = true;
                ops.virtual_swap(b2, b0);
            }

            ops.dump(out << "\n");
            CAPTURE(reversed, out.str());
            REQUIRE(ops.invariant());

            REQUIRE(b0.pos() == p2);
            REQUIRE(b2.pos() == p0);
        }
    }
}
