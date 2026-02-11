#include <catch2/catch_all.hpp>

#include <estd/internal/units/ostream.h>
#include <estd/string_view.h>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"


using namespace embr::mem;

namespace estd { namespace units { inline namespace v1 { namespace detail {

// ADL you are a demanding one.  OK, here you go
//template <class Traits>
//std::ostream& operator<<(std::ostream& out, const unit<Traits>& v)
template <class Rep, class Period>
std::ostream& operator<<(std::ostream& out, embr::mem::bytes_unit<Rep, Period> v)
{
    // TODO: Make a helper for this in estd, perhaps another flag for put_unit
    if(Period::num == Period::den || v.count() == 0)
        out << put_unit(bytes<int>(v));
    else
        out << v.count() << " (" << put_unit(bytes<int>(v)) << ')';

    return out;
}

}}}}


constexpr unsigned s_l1_pool_sz = 512;
using s_l1_pool_type = v1::layer1::pool<s_l1_pool_sz, 8>;
s_l1_pool_type s_pool;

TEST_CASE("gc mem v1 tests", "[memory][gc]")
{
    SECTION("detail")
    {
        SECTION("detail::pool: raw array/layer1")
        {
            constexpr unsigned pool_size = 2048;
            using page = detail::v1::page<uint16_t>;
            using handles_traits = detail::v1::handles_traits<page[10]>;
            using pool_traits = detail::v1::pool_traits<char[pool_size]>;
            using bytes = estd::units::bytes<int>;

            using pool_ops_traits = detail::v1::pool_ops_traits<pool_traits, handles_traits>;
            using ops_type = detail::v1::pool_ops<pool_ops_traits>;
            ops_type op;

            // DEBT: Need to make this automatic
            op.reset();

            SECTION("ops")
            {
                using pos_type = ops_type::pos_type;
                using bundle = ops_type::bundle;
                using block = ops_type::block;

                // synthetic size of to-be-allocated block, in aliased units (actually 64 bytes
                // since aliasing is 8)
                constexpr pos_type phys_sz(8);

                // DEBT: Brute force check that operator << overload above is happy.  Do a real stringstream
                // check instead
                //REQUIRE(phys_sz == pos_type(0));

                SECTION("alloc")
                {
                    constexpr bytes block_sz = block::header_size(block::Trivial);
                    bundle bn = op.alloc(phys_sz, block::Trivial);

                    REQUIRE(bn.invariant());
                    REQUIRE(bn.is_null() == false);
                    REQUIRE(bn.block->allocated());
                    REQUIRE(bn.handle == 0);
                    REQUIRE(bn.block->next() == 1);
                    REQUIRE(bn.has_prev() == false);

                    bn = op.next(bn);
                    REQUIRE(bn.allocated() == false);
                    REQUIRE(bn.handle == 1);
                    REQUIRE(bn.block->prev() == 0);
                    REQUIRE(bn.has_next() == false);

                    constexpr bytes logical_alloced_sz = phys_sz - block_sz;
                    constexpr bytes phys_alloced_sz = block_sz + phys_sz;

                    REQUIRE(op.available() == pool_size - phys_alloced_sz.count());
                    REQUIRE(op.alloced() == logical_alloced_sz);

                    bn = op.alloc(phys_sz, block::Trivial);
                    REQUIRE(bn.handle == 1);
                    REQUIRE(bn.block->prev() == 0);
                    REQUIRE(bn.has_next());
                    REQUIRE(bn.invariant());

                    // Trailing big free block
                    bn = op.next(bn);
                    REQUIRE(bn.allocated() == false);
                    REQUIRE(bn.handle == 2);
                    REQUIRE(bn.block->prev() == 1);
                    REQUIRE(bn.has_next() == false);
                    REQUIRE(bn.invariant());
                }
                SECTION("realloc")
                {
                    bundle bn = op.alloc(phys_sz, block::Trivial);
                    block* bl = bn.block;
                    page* p = bn.page;
                    pos_type pos = p->pos();
                    pos_type new_phys_sz = phys_sz + pos_type(1);
                    REQUIRE(pos.count() == 0);

                    memcpy(op.lock(bn.handle), "Hello", 6);
                    op.unlock(bn.handle);

                    SECTION("simple grow")
                    {
                        // A F pattern, ideally we expect this to expand existing A and shrink existing F.  At this point we're
                        // not there yet so it's going to move the block

                        bool r = op.realloc(bn, new_phys_sz);

                        REQUIRE(op.invariant());

                        REQUIRE(r);

                        // Block does NOT move for a simpler grow
                        REQUIRE(bn.block == bl);

                        pos_type sz2 = op.phys_size(bn);

                        REQUIRE(sz2 == new_phys_sz);
                    }
                    SECTION("move")
                    {
                        // Allocate another block to inhibit grow-into-free
                        op.alloc(phys_sz, block::Trivial);

                        bool r = op.realloc(bn, new_phys_sz);

                        REQUIRE(op.invariant());

                        REQUIRE(r);

                        // Block has moved for this bundle, so re-acquire it
                        bn = op.get_bundle(bn.handle);

                        REQUIRE(bn.page->pos() != pos);
                        REQUIRE(bn.block != bl);
                        REQUIRE(bn.is_null() == false);
                        REQUIRE(bn.allocated() == true);
                        REQUIRE(estd::string_view((char*)op.lock(bn.handle)) == "Hello");
                        op.unlock(bn.handle);

                        pos_type sz2 = op.phys_size(bn);

                        REQUIRE(sz2 == new_phys_sz);
                    }
                }
                SECTION("copy")
                {
                    int counter = 0;
                    constexpr block::modes mode = detail::v1::ascertain_block_mode<SideEffector>();

                    bundle bn = op.construct<mode, SideEffector>(&counter);

                    REQUIRE(bn.allocated());

                    // DEBT: Broaching on UB to copy a locked block.  Acceptable though.
                    auto se = (SideEffector*)op.lock(bn);

                    REQUIRE(se->copied_from_counter == 0);

                    bundle copied = op.copy(bn);

                    auto se_copied = (SideEffector*)op.lock(copied);

                    REQUIRE(se->copied_from_counter == 1);
                    REQUIRE(se_copied->counter_ == &counter);
                    REQUIRE(se_copied->copied_to_counter == 1);

                    op.unlock(bn.handle);
                }
            }
            SECTION("construct")
            {
                ops_type::storage_type& storage = const_cast<ops_type::storage_type&>(op.storage());
                ops_type::handles_type& handles = const_cast<ops_type::handles_type&>(op.handles());

                int counter = 0;
                int h = detail::construct<SideEffector>(storage, handles, &counter);

                REQUIRE(counter == 1);

                op.dealloc(h);

                REQUIRE(counter == 0);
            }
        }
    }
    SECTION("layer1")
    {
        constexpr unsigned pool_sz = 512;
        using pool_type = v1::layer1::pool<pool_sz, 8>;
        using handle_type = pool_type::handle_type;
        using bundle = pool_type::ops_type::bundle;
        using lock_handle = detail::lock_handle<pool_type, nullptr>;
        pool_type pool1;
        using block = detail::v1::block_8;
        constexpr unsigned block_sz = block::header_size(block::Trivial).count();

        int counter = 0;

        // FIX: All these shared_handle MAY want to participate in zero'ing lock counter
        // too?  What happens if we call .destroy() on something that is locked?
        // I think that ought to be a runtime error.

        SECTION("basic")
        {
            handle_type h1 = pool1.alloc(10);
            REQUIRE((int)h1 == 0);
            void* locked = pool1.lock(0);
            // We allocate from very start of pool, and lock returns user/app data just past block header
            REQUIRE(locked == pool1.ops().storage().data() + block_sz);
            pool1.unlock(0);
        }
        SECTION("realloc")
        {
            handle_type h0 = pool1.alloc(10);
            lock_handle lh0(h0, &pool1);
            memcpy(lh0.guard().data(), "Hello", 6);
            pool1.realloc(h0, 32);
            REQUIRE(pool1.allocated() == 32);
        }
        SECTION("defrag")
        {

        }
        SECTION("shared_handle")
        {
            REQUIRE(pool1.ops().available() == pool_sz - block_sz);

            // trivial
            {
                pool_type::handle_type h1 = pool1.alloc(block_sz);
                bundle bn1 = pool1.ops().get_bundle(h1);
                detail::v1::shared_handle<pool_type> sh1(h1, &pool1);

                REQUIRE(pool1.ops().available() == pool_sz - block_sz * 3);

#if !__cpp_deduction_guides
                // UNTESTED
                using lock_guard = detail::v1::lock_guard<pool_type, nullptr>;
#endif

                REQUIRE(bn1.block->lock_count() == 0);
                {
                    lock_guard guard{sh1};
                    REQUIRE(bn1.block->lock_count() == 1);
                }
                REQUIRE(bn1.block->lock_count() == 0);
            }

            {
                pool_type::handle_type h1 = pool1.construct<SideEffector>(&counter);
                bundle bn1 = pool1.ops().get_bundle(h1);
                v1::shared_handle<SideEffector, pool_type> sh1(h1, &pool1);
                using guard_type = decltype(sh1)::guard_type;

                SideEffector* se = sh1.lock();

                REQUIRE(se->counter_ == &counter);

                sh1.unlock();

                REQUIRE(counter == 1);

                REQUIRE(bn1.block->lock_count() == 0);

                {
                    auto guard = sh1();

                    ++*guard->counter_;

                    REQUIRE(bn1.block->lock_count() == 1);
                }

                {
                    guard_type guard = sh1;
                }

                {
                    guard_type guard(sh1);
                }

                {
                    ++*sh1()->counter_;
                }

                REQUIRE(counter == 3);

                {
                    --*(*sh1()).counter_;
                }

                REQUIRE(bn1.block->lock_count() == 0);

                REQUIRE(counter == 2);
            }

            {
                v1::shared_handle<SideEffector, pool_type> sh1 =
                    v1::make_shared<SideEffector>(pool1, &counter);

                REQUIRE(counter == 2);
            }

            REQUIRE(counter == 1);
            REQUIRE(pool1.ops().available() == pool_sz - block_sz);
            REQUIRE(pool1.ops().invariant());
        }
        SECTION("shared_handle (global)")
        {
            pool_type& pool2 = s_pool;

            pool2.ops().reset();

            // trivial
            {
                pool_type::handle_type h1 = pool2.alloc(block_sz);
                detail::v1::shared_handle<pool_type, &s_pool> sh1(h1);
                REQUIRE(pool2.ops().available() == pool_sz - block_sz * 3);
            }

            {
                pool_type::handle_type h1 = pool2.construct<SideEffector>(&counter);
                v1::shared_handle<SideEffector, pool_type, &s_pool> sh1(h1);

                static_assert(sizeof(sh1) == sizeof(pool_type::handle_type));

                SideEffector* se = sh1.lock();

                REQUIRE(se->counter_ == &counter);

                sh1.unlock();

                REQUIRE(counter == 1);
            }

            REQUIRE(counter == 0);

            REQUIRE(pool2.ops().available() == pool_sz - block_sz);
        }
        SECTION("unique_handle")
        {
            {
                pool_type::handle_type h1 = pool1.construct<SideEffector>(&counter);
                detail::v1::unique_handle<pool_type, nullptr> uh1(h1, &pool1);

                REQUIRE(counter == 1);
            }

            REQUIRE(counter == 0);

            {
                pool_type::handle_type h1 = pool1.construct<SideEffector>(&counter);
                unique_handle<SideEffector, pool_type> uh1(h1, &pool1);

                REQUIRE(counter == 1);
            }

            REQUIRE(counter == 0);
        }
    }
    SECTION("layer3")
    {
        layer3::pool::page_type pages[20];
        char buf[512];

        layer3::pool pool1(pages, buf);
    }
}
