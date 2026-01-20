#include <catch2/catch_all.hpp>

#include <estd/internal/units/ostream.h>

#include <embr/mem/v1/pool.hpp>

#include "test-mem-data.h"


using namespace embr::mem;


template <class Traits, class HandlesTraits, std::size_t N>
static void assemble_pool(typename detail::pool<Traits>::template ops<HandlesTraits>& ops, const test::page (&pool)[N])
{
    using page_type = typename HandlesTraits::value_type;
    using pos_type = typename page_type::unit_type;
    const auto aliasing = ops.aliasing;
    const auto null = HandlesTraits::null;
    int prev = null;

    pos_type tally(0);

    for(unsigned i = 0, j = 0; j < N; ++j)
    {
        const test::page& page = pool[j];

        int next = j == N - 1 ? null : page.blk.next() == null ?
            i + 1 : page.blk.next();

        ops.handles_[i].pos(tally);
        detail::bundle b = ops.get_bundle(i);

        *b.block = detail::block(page.blk.mode(), page.blk.allocated(), prev, next);

        tally += pos_type(page.phys_sz / aliasing);

        prev = i;
        i = next;
    }
}

TEST_CASE("gc mem v1 edge cases", "[memory][gc]")
{
    constexpr unsigned pool_size = 2048;
    using page = detail::v1::page<uint16_t>;
    using handles_traits = detail::v1::handles_traits<page[10]>;
    using handles_type = detail::v1::handles<handles_traits>;
    using pool_traits = detail::v1::pool_traits<char[pool_size]>;
    using pool_type = detail::v1::pool<pool_traits>;

    handles_type handles;
    pool_type pool;

    // DEBT: Need to make this automatic
    pool.reset(handles);

    using bundle = detail::v1::bundle;
    using block = detail::v1::block;
    using ops_type = pool_type::ops<handles_traits>;
    ops_type op{pool, handles};

    SECTION("pool assembly/edge cases")
    {
        detail::fragmentation frag;
        detail::fragmentation::candidate& frag0 = frag.candidates[0];

        std::ostringstream before;

        before << "\nbefore:\n";

        std::ostringstream out;

        out << "\n";

        SECTION("assemble_pool itself")
        {
            assemble_pool<pool_traits>(op, test::pool1);

            REQUIRE(op.alloced() == 24);
            unsigned available = op.available();
            REQUIRE(available == pool_size - (32 + 8));
        }
        SECTION("defrag case 2: reverse overlapping trivial movement")
        {
            // NOTE: Doesn't match defrag failure pool size (that one's 512)
            assemble_pool<pool_traits>(op, test::pool2);
            detail::const_bundle from, to;

            from = op.get_bundle(2);
            to = op.get_bundle(3);

            frag0 = {48, from, to, true};

            op.dump(before);
            CAPTURE(before.str());

            op.defrag(frag0);

            op.dump(out);

            CAPTURE(out.str());
            REQUIRE(op.invariant());

            unsigned alloced = op.alloced();
            REQUIRE(alloced == 16 + 56 + 40 + 32);

            from = op.get_bundle(2);
            to = op.get_bundle(3);

            REQUIRE(to.page->is_null() == true);
            REQUIRE(from.page->is_null() == false);
            REQUIRE(from.allocated());
        }
        SECTION("defrag case 3: reverse overlapping trivial movement")
        {
            assemble_pool<pool_traits>(op, test::pool3);

            detail::const_bundle from, to;

            from = (op.get_bundle(4));
            to = (op.get_bundle(3));

            detail::fragmentation::candidate frag0{171, from, to, true};

            op.dump(before);

            op.defrag(frag0);

            op.dump(out);

            CAPTURE(before.str(), out.str());
            REQUIRE(op.invariant());
        }
        SECTION("defrag case 4: reverse non-overlapping trivial")
        {
            assemble_pool<pool_traits>(op, test::pool4);

            detail::const_bundle from, to;

            op.dump(before);

            from = (op.get_bundle(6));
            to = (op.get_bundle(2));

            frag0 = {171, from, to, false};

            op.defrag(frag0);

            op.dump(out);

            CAPTURE(before.str(), out.str());
            REQUIRE(op.invariant());
        }
        SECTION("defrag case 5: big-free-block unselect")
        {
            constexpr unsigned pool_size = 512;
            //using handles_traits = detail::v1::handles_traits<page[8]>;
            using handles_type = detail::v1::handles<handles_traits>;
            using pool_traits = detail::v1::pool_traits<char[pool_size]>;
            using pool_type = detail::v1::pool<pool_traits>;
            using ops_type = pool_type::ops<handles_traits>;

            pool_type pool;

            ops_type op{pool, handles};

            assemble_pool<pool_traits>(op, test::pool5);

            op.dump(before);

            op.assess(&frag);

            // Without tuning, assess will select last free block since it's smaller in this case.
            // However, we favor the last big-free-block paradigm.  Ensure that we move 6 -> 3
            // so that 6 and 7 can merge into big free block

            REQUIRE(frag0.bundle.handle == 6);
            REQUIRE(frag0.move_to.handle == 3);

            op.defrag(frag0);

            CAPTURE(before.str(), out.str());
            REQUIRE(op.invariant());
        }
        SECTION("defrag case 6: allocated block trailing moved-to")
        {
            assemble_pool<pool_traits>(op, test::pool6);

            op.dump(before);

            frag0 = {67, op.get_bundle(1), op.get_bundle(2), false, true};

            CAPTURE(before.str());

            op.defrag(frag0);
        }
        SECTION("defrag case 7: split")
        {
            assemble_pool<pool_traits>(op, test::pool7);

            op.dump(before);

            frag0 = {54, op.get_bundle(1), op.get_bundle(2), false, true};

            CAPTURE(before.str());

            SECTION("relinked")
            {
                op.defrag(frag0, true);

                op.dump(out);

                CAPTURE(out.str());
                REQUIRE(op.invariant());
            }
            SECTION("non-relinked")
            {
                op.defrag(frag0, false);

                op.dump(out);

                CAPTURE(out.str());

                bundle bn0 = op.get_bundle(0);
                bundle bn1 = op.get_bundle(1);

                REQUIRE(bn0.allocated() == false);
                REQUIRE(bn1.allocated() == false);
                REQUIRE(op.invariant());
            }
        }
        SECTION("defrag case 8: overlap + relink")
        {
            assemble_pool<pool_traits>(op, test::pool8);

            op.dump(before);

            CAPTURE(before.str());

            frag0 = {62, op.get_bundle(2), op.get_bundle(3), true, true};

            // Moves 2->3 (overlapping) resulting in expanded 0
            // Fails in that:
            // #0 prev link is goofed up to be '2' instead of 0xFF
            // #2 prev link is goofed up to be '3' instead of staying 0 as it should
            // In an overlap case like this, a handle swap and relink is not appropriate since
            // handle 2 is not moving enough to break contiguous nature of next/prev, and
            // handle 3 vanishes

            // Early relink ON mode, eventually flag will phase out
            op.defrag(frag0, true);

            bundle bn2 = op.get_bundle(2);

            op.dump(out);

            CAPTURE(out.str());
            REQUIRE(op.invariant());

            //REQUIRE(bn2.allocated());
        }
        SECTION("defrag case 9: overlap - reverse direction")
        {
            assemble_pool<pool_traits>(op, test::pool9);

            op.dump(before);

            CAPTURE(before.str());

            bundle from = op.get_bundle(6);
            bundle to = op.get_bundle(5);

            frag0 = {1933, from, to, true, false};

            op.defrag(frag0);

            op.dump(out);

            CAPTURE(out.str());

            from = op.get_bundle(6);

            REQUIRE(from.page->is_null() == false);

            // TODO: Not resolved yet
            //REQUIRE(from.allocated());
        }
    }
}
