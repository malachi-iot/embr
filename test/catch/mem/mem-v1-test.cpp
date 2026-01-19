#include <catch2/catch_all.hpp>

#include <estd/internal/units/ostream.h>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"


using namespace embr::mem;

namespace estd { namespace units { inline namespace v1 { namespace detail {

template <class Rep, class Period>
using page_unit_type2 = unit<embr::mem::detail::v1::page_unit_traits<Rep, Period>>;

// ADL you are a demanding one.  OK, here you go
//template <class Traits>
//std::ostream& operator<<(std::ostream& out, const unit<Traits>& v)
template <class Rep, class Period>
std::ostream& operator<<(std::ostream& out, page_unit_type2<Rep, Period> v)
{
    out << v.count();

    if(Period::num != Period::den)
    {
        // not 1:1 means let's do a 1:1 (pure bytes) one also
        // DEBT: OK I already did this elsewhere... probably should standardize this in estd but only
        // activate with some kind of feature flag.  That includes the whole suffix thing
        // See https://github.com/malachi-iot/estdlib/issues/172
        unit<embr::mem::detail::v1::page_unit_traits<double, estd::ratio<1>>> u(v);

        out << " (" << u.count() << " bytes)";
    }

    return out;
}

}}}}




TEST_CASE("gc mem v1 tests", "[memory][gc]")
{
    SECTION("detail")
    {
        SECTION("detail::pool: raw array/layer1")
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

            SECTION("ops")
            {
                using bundle = detail::v1::bundle;
                using block = detail::v1::block;
                using ops_type = pool_type::ops<handles_traits>;
                using pos_type = ops_type::pos_type;
                ops_type op{pool, handles};

                // synthetic size of to-be-allocated block
                constexpr pos_type phys_sz(8);
                constexpr unsigned phys_sz_bytes = ops_type::aliasing * phys_sz.count();

                SECTION("alloc")
                {
                    constexpr unsigned block_sz = block::header_size(block::Trivial);
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

                    constexpr unsigned logical_alloced_sz = phys_sz_bytes - block_sz;
                    constexpr unsigned phys_alloced_sz = block_sz + phys_sz_bytes;

                    REQUIRE(op.available() == pool_size - phys_alloced_sz);
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
                        REQUIRE(std::string_view((char*)op.lock(bn.handle)) == "Hello");
                        op.unlock(bn.handle);

                        pos_type sz2 = op.phys_size(bn);

                        REQUIRE(sz2 == new_phys_sz);
                    }
                }
            }
            SECTION("alloc")
            {
                pool.alloc(handles, 32);
            }
            SECTION("construct")
            {
                int counter = 0;
                int h = detail::construct<SideEffector>(pool, handles, &counter);

                REQUIRE(counter == 1);

                pool.dealloc(handles, h);

                REQUIRE(counter == 0);
            }
        }
    }
    SECTION("layer1")
    {
        constexpr unsigned pool_sz = 512;
        using pool_type = v1::layer1::pool<pool_sz, 8>;
        using bundle = pool_type::ops_type::bundle;
        pool_type pool1;
        using block = detail::v1::block;
        constexpr unsigned block_sz = block::header_size(block::Trivial);

        // DEBT: This guy still being stupid
        pool1.ops().reset();

        int counter = 0;

        // FIX: All these shared_handle MAY want to participate in zero'ing lock counter
        // too?  What happens if we call .destroy() on something that is locked?
        // I think that ought to be a runtime error.

        SECTION("basic")
        {
            pool_type::handle_type h1 = pool1.alloc(10);
            REQUIRE((int)h1 == 0);
            void* locked = pool1.lock(0);
            // We allocate from very start of pool, and lock returns user/app data just past block header
            REQUIRE(locked == pool1.ops().self_.data() + block_sz);
            pool1.unlock(0);
        }
        SECTION("realloc")
        {

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
                detail::v1::shared_handle<pool_type, nullptr> sh1(h1, &pool1);

                REQUIRE(pool1.ops().available() == pool_sz - block_sz * 3);

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
                    lock_guard guard = sh1;
                }

                {
                    lock_guard guard(sh1);
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
            static pool_type pool2;

            pool2.ops().reset();

            // trivial
            {
                pool_type::handle_type h1 = pool2.alloc(block_sz);
                detail::v1::shared_handle<pool_type, &pool2> sh1(h1);
                REQUIRE(pool2.ops().available() == pool_sz - block_sz * 3);
            }

            {
                pool_type::handle_type h1 = pool2.construct<SideEffector>(&counter);
                v1::shared_handle<SideEffector, pool_type, &pool2> sh1(h1);

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
    SECTION("layer2")
    {

    }
}
