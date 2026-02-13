#include <catch2/catch_all.hpp>

#include <sstream>

#include <estd/string_view.h>

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
    using handle_type = handles_type::size_type;
    using pool_traits = detail::v1::pool_traits<estd::span<char>>;
    using pool_type = detail::v1::pool<pool_traits>;
    using ops_type = detail::pool_ops<detail::pool_ops_traits<pool_traits, handles_traits>>;
    using block = ops_type::block;
    using bundle = ops_type::bundle;
    using const_bundle = ops_type::const_bundle;
    using pos_type = page::unit_type;

    static_assert(std::is_same<pool_type, ops_type::storage_type>::value);

    char buf[pool_size];

    ops_type ops{buf};

    // DEBT: Still having to do this
    ops.reset();

    SECTION("bundle things")
    {
        bundle bn1;
        const_bundle cbn(bn1);
        // Correctly kicks back
        //bundle bn2(cbn);
    }
    SECTION("detail")
    {
        SECTION("page")
        {
            REQUIRE(page::null == 0xFFFF);
        }
        SECTION("handles: layer1")
        {
#if PAGE_ALIAS
            using unit_type = page;
#else
            using unit_type = page::unit_type;
#endif
            using type = detail::v1::handles<detail::v1::handles_traits<page[20]>>;
            using handle_type = unsigned;

            static_assert(sizeof(type) == 20 * 2);

            type handles;

            REQUIRE(handles[0].is_null());

            handle_type h0 = handles.alloc(
                [](int i, const page&) { return true; },
                [](int i, page& v)
                {
                    v.pos(unit_type(1));
                });

            REQUIRE(h0 == 0);

            //detail::v1::bundle bundle{nullptr, nullptr, h0};

            // DEBT: https://github.com/malachi-iot/estdlib/issues/159
            REQUIRE(handles.dealloc(h0) == estd::errc{});

            //type::iterator b = handles.begin();
        }
        SECTION("handles: layer2")
        {
            using page = detail::v1::page<uint16_t, estd::ratio<sizeof(void*)>>;
            using unit_type = page::unit_type;
            using type = detail::v1::handles<detail::v1::handles_traits<estd::span<page, 20>>>;
            page backing[20];

            static_assert(sizeof(type) == sizeof(page*));

            type handles(backing);

            REQUIRE(handles[0].is_null());
        }
        SECTION("block: RttoBase")
        {
            int counter{};
            using block = detail::v1::block_8;

            union
            {
                block b1;
                char storage1[64];
            };

            union
            {
                block b2;
                char storage2[64];
            };

            b1.reset(block::RttoBase, true);
            b1.emplace<RttoSideEffector>(&counter);
            b2.reset(block::Trivial, false);

            REQUIRE(counter == 1);

            SECTION("copy")
            {
                b2.copy_from(&b1, sizeof(RttoSideEffector));
                REQUIRE(b2.mode() == block::RttoBase);
                auto rse2 = (RttoSideEffector*)b2.data();       // NOLINT
                REQUIRE(rse2->copied_to_counter == 1);
                b1.destroy();
            }
            SECTION("move")
            {

            }
        }
        SECTION("block: RttoProxy")
        {
            int counter{};
            using block = detail::v1::block_8;
            using proxy = estd::internal::rtto_base::base;

            union
            {
                block b1;
                char storage1[64]{};
            };

            union
            {
                block b2;
                char storage2[64]{};
            };

            b1.reset(block::RttoProxy, true);

            SECTION("block: move")
            {
                b1.emplace_rtto_proxied<SideEffector>(&counter);

                REQUIRE(counter == 1);

                b2.move_from(&b1, sizeof(SideEffector));

                b1.proxy()->destroy();
                b2.proxy()->destroy();

                REQUIRE(counter == 0);
            }
            SECTION("block: copy")
            {
                b1.emplace_rtto_proxied<SideEffector>(&counter);

                auto se2 = (SideEffector*) b2.proxy()->storage();

                // DEBT: Slightly UB-ish since we never actually constructed b2
                REQUIRE(se2->copied_to_counter == 0);

                b2.copy_from(&b1, sizeof(SideEffector));

                REQUIRE(se2->copied_to_counter == 1);

                b1.proxy()->destroy();
                b2.proxy()->destroy();

                // No move means --*counter happens twice
                REQUIRE(counter == -1);
            }
        }
    }
    SECTION("filter_iterator")
    {
        static constexpr int values[] { 0, 4, 7, 9, 3, 6 };
        static constexpr auto end = std::end(values);

        SECTION("lambda filter")
        {
            auto filter = [](const int& v)
            {
                // 'end' value is always included to avoid tumbling into undefined memory
                if(&v == end) return true;

                return v > 5;
            };
            using iterator = v1::filter_iterator<decltype(filter), const int*>;
            iterator i{values};

            REQUIRE(*i == 7);
            ++i;
            REQUIRE(*i++ == 9);
            REQUIRE(*i == 6);

            bool r = ++i == end;

            REQUIRE(r);
        }
        SECTION("generic_end_predicate")
        {

        }
    }
    SECTION("virtual_swap")
    {
        bundle b0 = ops.alloc(pos_type(8), block::Trivial);
        bundle b1 = ops.get_bundle(1);
        pos_type p0 = b0.pos();
        pos_type p1 = b1.pos();
        std::ostringstream out;

        REQUIRE(b0.handle == 0);
        REQUIRE(b0.allocated());
        REQUIRE(b1.allocated() == false);

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
    SECTION("first_free")
    {
        pos_type found_size(0);
        constexpr pos_type phys_sz(8);
        const_bundle bn = ops.first_free(phys_sz, &found_size);

        REQUIRE(bn.invariant());
        REQUIRE(bn.is_null() == false);
        REQUIRE(bn.allocated() == false);
    }
    SECTION("assess")
    {
        constexpr pos_type phys_sz(8);
        constexpr unsigned phys_sz_bytes = ops_type::aliasing * phys_sz.count();
        constexpr unsigned block_sz = block::header_size(block::Trivial).count();
        ops_type::fragmentation frag{};
        auto& frag0 = frag.candidates[0];
        auto& frag1 = frag.candidates[1];
        void* data;
        ops_type& op = ops;

        op.assess(&frag);

        // No allocations ever = no fragmentation
        REQUIRE(frag0.bundle.is_null() == true);

        // Allocates 64 physical bytes (56 logical)
        // This leaves 2048 - 64 = 1984 (1976 logical)
        bundle bn = op.alloc(phys_sz, block::Trivial);

        REQUIRE(op.logical_size(bn) == 56);

        // Allocates 64 physical bytes again (56 logical)
        // This leaves 2048 - 128 = 1920 (1912 logical)
        bn = op.alloc(phys_sz, block::Trivial);
        // Deallocates 64 physical bytes (56 logical)
        // This leaves 2048 - 64 = 1984 (1976 logical)
        op.dealloc(0);
        REQUIRE(op.invariant());

        // Now we have a F A F pattern

        // 2048 - (2 free blocks headers) - (allocated physical block size)
        REQUIRE(op.available() == pool_size - phys_sz_bytes - block_sz * 2);

        memcpy(data = op.lock(bn), "Hello", 6);
        op.unlock(bn.handle);

        op.assess(&frag);

        REQUIRE(frag.largest_free_handle == 2);
        REQUIRE(frag0.overlap == false);
        REQUIRE(frag0.bundle.allocated());
        REQUIRE(frag0.bundle.is_null() == false);
        REQUIRE(frag0.bundle.handle == 1);
        REQUIRE(frag0.move_to.handle == 0);

        handle_type alloced = 1;
        handle_type free_block = 0;
        bool relink_mode = false;

        // Since we correctly identified handle 1 is allocated and fragmented, as identified
        // by frag0, defrag (move it)

        SECTION("non-relinked")
        {
            alloced = 0;
            free_block = 1;
        }
        SECTION("relinked")
        {
            relink_mode = true;
        }

        CAPTURE(relink_mode, alloced, free_block);

        op.defrag(frag0, relink_mode);
        REQUIRE(op.invariant());

        bn = op.get_bundle(free_block);

        // handle 1 now ought to be freed up, since above frag assessment indicated to move
        // allocated 1 elsewhere
        REQUIRE(bn.is_null() == false);
        REQUIRE(bn.allocated() == false);
        // handle 1 merged with old handle 2, now claiming the throne as the big free block
        REQUIRE(bn.has_next() == false);
        REQUIRE(op.logical_size(bn) == pool_size - phys_sz_bytes - block_sz);
        REQUIRE(bn.invariant());

        bn = op.get_bundle(alloced);

        // handle 0 was determined the move_to destination, which means he should
        // have the old handle 1 data
        REQUIRE(bn.is_null() == false);
        REQUIRE(data != op.lock(bn));
        REQUIRE(estd::string_view((char*)op.lock(bn)) == "Hello");
        REQUIRE(bn.allocated());
        REQUIRE(bn.invariant());
        REQUIRE(op.invariant());

        op.unlock(bn.handle);
        op.unlock(bn.handle);
    }
    SECTION("move")
    {
        constexpr pos_type phys_sz(8);
        constexpr unsigned phys_sz_bytes = ops_type::aliasing * phys_sz.count();
        ops_type& op  = ops;;
        bundle bn = op.alloc(phys_sz, block::Trivial);

        memcpy(op.lock(bn.handle), "Hello", 6);
        op.unlock(bn.handle);

        SECTION("to BFB")
        {
            //block* b = bn.block;
            page* p = bn.page;
            pos_type pos = p->pos();
            bundle bn_free = op.get_bundle(1);

            REQUIRE(bn.handle == 0);
            REQUIRE(bn.has_next());
            REQUIRE(bn_free.has_next() == false);
            REQUIRE(bn_free.allocated() == false);

            op.move(bn, bn_free, 0, 24, false);

            bn_free = op.get_bundle(0);

            REQUIRE(bn_free.allocated() == false);

            bn = op.get_bundle(1);

            // Ensure split did occur
            REQUIRE(bn.has_next());
            REQUIRE(bn.allocated());
            REQUIRE(bn.page->pos() != pos);

            REQUIRE(estd::string_view((char*)op.lock(bn.handle)) == "Hello");
            op.unlock(bn.handle);

            REQUIRE(op.phys_size(bn) == pos_type(4));
        }
    }
}
