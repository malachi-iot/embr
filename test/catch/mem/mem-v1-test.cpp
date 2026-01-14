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


TEST_CASE("gc mem v1 tests", "[memory][gc]")
{
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
    SECTION("detail")
    {
        SECTION("page")
        {
            using page = detail::v1::page<uint16_t>;

            REQUIRE(page::null == 0xFFFF);
        }
        SECTION("handles: layer1")
        {
            using page = detail::v1::page<uint16_t>;
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
            REQUIRE(handles.dealloc(h0) == estd::errc::values{});

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
        SECTION("block")
        {
            int counter{};
            using block = detail::v1::block;
            using proxy = estd::internal::rtto_base::base;

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

            b1.reset(block::RttoProxy, false);
            b1.emplace_rtto_proxied<SideEffector>(&counter);

            REQUIRE(counter == 1);

            b2.move_from(&b1, sizeof(SideEffector));

            b1.proxy()->destroy();
            b2.proxy()->destroy();

            REQUIRE(counter == 0);
        }
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

                SECTION("first_free")
                {
                    pos_type found_size(0);
                    bundle bn = op.first_free(phys_sz, &found_size);

                    REQUIRE(bn.invariant());
                    REQUIRE(bn.is_null() == false);
                    REQUIRE(bn.allocated() == false);
                }
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
                SECTION("assess")
                {
                    constexpr unsigned block_sz = block::header_size(block::Trivial);
                    detail::fragmentation frag{};
                    auto& frag0 = frag.candidates[0];
                    auto& frag1 = frag.candidates[1];
                    void* data;

                    op.assess(&frag);

                    // No allocations ever = no fragmentation
                    REQUIRE(frag0.bundle.is_null() == true);

                    bundle bn = op.alloc(phys_sz, block::Trivial);
                    bn = op.alloc(phys_sz, block::Trivial);
                    op.dealloc(0);
                    REQUIRE(op.invariant());

                    memcpy(data = op.lock(bn), "Hello", 6);
                    op.unlock(bn.handle);

                    // Now we have a F A F pattern

                    op.assess(&frag);

                    REQUIRE(frag.largest_free_handle == 2);
                    REQUIRE(frag0.overlap == false);
                    REQUIRE(frag0.bundle.allocated());
                    REQUIRE(frag0.bundle.is_null() == false);
                    REQUIRE(frag0.bundle.handle == 1);
                    REQUIRE(frag0.move_to.handle == 0);

                    // Since we correctly identified handle 1 is allocated and fragmented, as identified
                    // by frag0, defrag (move it)
                    op.defrag(frag0);
                    REQUIRE(op.invariant());

                    bn = op.get_bundle(1);

                    // handle 1 now ought to be freed up, since above frag assessment indicated to move
                    // allocated 1 elsewhere
                    REQUIRE(bn.is_null() == false);
                    REQUIRE(bn.allocated() == false);
                    // handle 1 merged with old handle 2, now claiming the throne as the big free block
                    REQUIRE(bn.has_next() == false);
                    REQUIRE(op.logical_size(bn) == pool_size - phys_sz_bytes - block_sz);
                    REQUIRE(bn.invariant());

                    bn = op.get_bundle(0);

                    // handle 0 was determined the move_to destination, which means he should
                    // have the old handle 1 data
                    REQUIRE(bn.is_null() == false);
                    REQUIRE(data != op.lock(bn));
                    REQUIRE(std::string_view((char*)op.lock(bn)) == "Hello");
                    REQUIRE(bn.allocated());
                    REQUIRE(bn.invariant());
                    REQUIRE(op.invariant());

                    op.unlock(bn.handle);
                    op.unlock(bn.handle);
                }
                SECTION("realloc")
                {
                    bundle bn = op.alloc(phys_sz, block::Trivial);
                    pos_type new_phys_sz = phys_sz + pos_type(1);

                    bool r = op.realloc(bn, new_phys_sz);

                    REQUIRE(r);

                    // Block may have moved for this bundle, so re-acquire it
                    bn = op.get_bundle(bn.handle);

                    pos_type sz2 = op.phys_size(bn);

                    // Nope, still 8 units
                    //REQUIRE(sz2 == new_phys_sz);
                }
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

                        op.defrag(frag0);

                        op.dump(out);

                        CAPTURE(out.str());
                        REQUIRE(op.invariant());

                        unsigned alloced = op.alloced();
                        REQUIRE(alloced == 16 + 56 + 40 + 32);
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

                        op.defrag(frag0);

                        op.dump(out);

                        CAPTURE(out.str());
                        REQUIRE(op.invariant());
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
