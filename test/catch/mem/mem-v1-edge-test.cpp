#include <catch2/catch_all.hpp>

#include <estd/internal/units/ostream.h>

#include <embr/mem/v1/pool.hpp>

#include "test-mem-data.h"


using namespace embr::mem;


template <class Traits, std::size_t N>
static void assemble_pool(typename detail::pool_ops<Traits>& ops, const test::page (&pool)[N])
{
    using ops_type = typename detail::pool_ops<Traits>;
    using handles_type = typename ops_type::handles_type;
    using handles_traits = typename Traits::handles_traits;
    using page_type = typename handles_traits::value_type;
    using pos_type = typename page_type::unit_type;
    using bundle = typename ops_type::bundle;
    using block = detail::v1::block_8;
    const auto aliasing = ops.aliasing;
    const auto null = handles_traits::null;
    int prev = null;
    handles_type& handles = const_cast<handles_type&>(ops.handles());
    using bytes = estd::units::bytes<unsigned>;

    pos_type tally(0);

    for(unsigned i = 0, j = 0; j < N; ++j)
    {
        const test::page& page = pool[j];

        int next = j == N - 1 ? null : page.blk.next() == null ?
            i + 1 : page.blk.next();

        handles[i].pos(tally);
        bundle b = ops.get_bundle(i);

        *b.block = detail::block_8(page.blk.mode(), page.blk.allocated(), prev, next);

        // Had this idea to auto populate these guys, but he wants int* for SideEffector::counter_
        // I suppose we could pass that into this function... for the time being feeding it a static
        // just to get him online
        static int sideeffector_counter_kludge = 0;

        switch(page.blk.mode())
        {
            case block::Trivial:
                break;

            case block::RttoProxy:
#if __SIZEOF_POINTER__ == 8 && __SIZEOF_INT__ == 4
                b.block->template emplace_rtto_proxied<SideEffector>(&sideeffector_counter_kludge);
#endif
                break;

            case block::RttoBase:
#if __SIZEOF_POINTER__ == 8 && __SIZEOF_INT__ == 4
                b.block->template emplace<RttoSideEffector>(&sideeffector_counter_kludge);
#endif
                break;

            default:
                assert(false);
        }

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
    using pool_traits = detail::v1::pool_traits<char[pool_size]>;
    using pool_ops_traits = detail::v1::pool_ops_traits<pool_traits, handles_traits>;

    using ops_type = detail::v1::pool_ops<pool_ops_traits>;
    using pos_type = typename ops_type::pos_type;
    using bundle = ops_type::bundle;
    using block = ops_type::block;
    using fragmentation = ops_type::fragmentation;
    ops_type op;

    // DEBT: Need to make this automatic
    op.reset();

    SECTION("pool assembly/edge cases")
    {
        fragmentation frag;
        fragmentation::candidate& frag0 = frag.candidates[0];

        std::ostringstream before;

        before << "\nbefore:\n";

        std::ostringstream out;

        out << "\n";

        SECTION("assemble_pool itself")
        {
            assemble_pool(op, test::pool1);

            REQUIRE(op.alloced() == 24);
            unsigned available = op.available().count();
            REQUIRE(available == pool_size - (32 + 8));
        }
        SECTION("defrag case 2: reverse overlapping trivial movement")
        {
            // NOTE: Doesn't match defrag failure pool size (that one's 512)
            assemble_pool(op, test::pool2);
            ops_type::const_bundle from, to;

            from = op.get_bundle(2);
            to = op.get_bundle(3);

            frag0 = {48, from, to, true};

            op.dump(before);
            CAPTURE(before.str());

            op.defrag(frag0);

            op.dump(out);

            CAPTURE(out.str());
            REQUIRE(op.invariant());

            auto alloced = op.alloced();
            REQUIRE(alloced == 16 + 56 + 40 + 32);

            from = op.get_bundle(2);
            to = op.get_bundle(3);

            REQUIRE(to.page->is_null() == true);
            REQUIRE(from.page->is_null() == false);
            REQUIRE(from.allocated());
        }
        SECTION("defrag case 3: reverse overlapping trivial movement")
        {
            assemble_pool(op, test::pool3);

            ops_type::const_bundle from, to;

            from = (op.get_bundle(4));
            to = (op.get_bundle(3));

            fragmentation::candidate frag0{171, from, to, true};

            op.dump(before);

            op.defrag(frag0);

            op.dump(out);

            CAPTURE(before.str(), out.str());
            REQUIRE(op.invariant());
        }
        SECTION("defrag case 4: reverse non-overlapping trivial")
        {
            assemble_pool(op, test::pool4);

            ops_type::const_bundle from, to;

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
            using traits = detail::v1::pool_ops_val_traits<pool_type, handles_type&>;
            using ops_type = detail::v1::pool_ops<traits>;

            auto& handles = const_cast<handles_type&>(op.handles());

            ops_type op{estd::nullopt, handles};

            assemble_pool(op, test::pool5);

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
            assemble_pool(op, test::pool6);

            op.dump(before);

            frag0 = {67, op.get_bundle(1), op.get_bundle(2), false, true};

            CAPTURE(before.str());

            op.defrag(frag0);
        }
        SECTION("defrag case 7: split")
        {
            assemble_pool(op, test::pool7);

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
            assemble_pool(op, test::pool8);

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

            REQUIRE(bn2.allocated());
        }
        SECTION("defrag case 9: overlap - reverse direction, free block not swallowed")
        {
            assemble_pool(op, test::pool9);

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
            REQUIRE(from.allocated());
        }
        SECTION("defrag case 10: virtual_move into null page")
        {
            assemble_pool(op, test::pool10);

            op.dump(before);

            CAPTURE(before.str());

            bundle from = op.get_bundle(1);
            bundle to = op.get_bundle(2);

            frag0 = {67, from, to, false, true};

            op.defrag(frag0);

            op.dump(out);

            from = op.get_bundle(1);

            CAPTURE(out.str());
            REQUIRE(op.invariant());
            REQUIRE(from.page->is_null() == false);
            REQUIRE(from.allocated());
        }
        SECTION("defrag case 11: RttoProxy move")
        {
            assemble_pool(op, test::pool11);
            int counter = 0;

#if __SIZEOF_POINTER__ == 8 && __SIZEOF_INT__ == 4
            bundle bn = op.get_bundle(4);
            bn.block->emplace_rtto_proxied<SideEffector>(&counter);
            pos_type old_pos = bn.pos();
            auto se = bn.block->proxy()->storage<SideEffector*>();

            REQUIRE(se->counter_ == &counter);

            op.dump(before);

            CAPTURE(before.str());

            op.assess(&frag);

            REQUIRE(frag0.bundle.handle == 4);
            REQUIRE(frag0.move_to.handle == 5);

            pos_type new_pos = frag0.move_to.pos();

            REQUIRE(frag0.bundle.pos() == old_pos);
            REQUIRE(new_pos != old_pos);

            op.defrag(frag0);

            // Remember, cleverness dictates we retain our handle#
            bn = op.get_bundle(4);

            se = bn.block->proxy()->storage<SideEffector*>();

            REQUIRE(se->moved_to_counter == 1);
            REQUIRE(se->counter_ == &counter);
            REQUIRE(bn.pos() != old_pos);
            REQUIRE(bn.pos() == new_pos);
#endif
        }
        SECTION("defrag case 12: RttoBase move")
        {
            // In fact this scenario never had a specific issue, I made mistake during battery test logic itself.
            // Keeping anyway
            assemble_pool(op, test::pool12);
            int counter = 0;

            op.assess(&frag);

            REQUIRE(frag0.bundle.handle == 5);
            REQUIRE(frag0.move_to.handle == 4);

            op.defrag(frag0);
        }
    }
}
