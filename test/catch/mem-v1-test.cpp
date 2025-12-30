#include <catch2/catch_all.hpp>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>


// DEBT: See if we can scoop this from estd
struct SideEffector
{
    int* counter_{};

    SideEffector(SideEffector&& move_from) :
        counter_{move_from.counter_}
    {
        move_from.counter_ = nullptr;
    }

    explicit SideEffector(int* counter) : counter_{counter}
    {
        ++*counter_;
    }

    ~SideEffector()
    {
        if(counter_)   --*counter_;
    }
};

using namespace embr::mem;

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

            // DEBT: Explicit reset here rather than auto-zero on construction feels a little off
            handles.reset();

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

            handles.reset();
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

            b1.mode(block::RttoProxy);
            b1.emplace_rtto_proxied<SideEffector>(&counter);

            REQUIRE(counter == 1);

            b2.move_from(&b1, sizeof(SideEffector));

            b1.proxy()->destroy();
            b2.proxy()->destroy();

            REQUIRE(counter == 0);
        }
        SECTION("pool: layer1")
        {
            constexpr unsigned pool_size = 2048;
            using page = detail::v1::page<uint16_t>;
            using handles_traits = detail::v1::handles_traits<page[20]>;
            using handles_type = detail::v1::handles<handles_traits>;
            using pool_type = detail::v1::pool<detail::v1::pool_traits<char[pool_size]>>;
            using pos_type = pool_type::pos_type;

            handles_type handles;
            pool_type pool;

            // DEBT: Need to make this automatic
            pool.reset(handles);

            SECTION("ops")
            {
                using bundle = detail::v1::bundle;
                using block = detail::v1::block;
                using ops_type = pool_type::ops<handles_traits>;
                ops_type op{pool, handles};
                constexpr pos_type phys_sz(8);

                SECTION("first_free")
                {
                    pos_type found_size(0);
                    bundle bn = op.first_free(phys_sz, &found_size);

                    REQUIRE(bn.invariant());
                    REQUIRE(bn.is_null() == false);
                    REQUIRE(bn.block->allocated() == false);
                }
                SECTION("alloc")
                {
                    constexpr unsigned block_sz = block::header_size<block::Trivial>();
                    bundle bn = op.alloc<block::Trivial>(phys_sz);

                    REQUIRE(bn.invariant());
                    REQUIRE(bn.is_null() == false);
                    REQUIRE(bn.block->allocated());

                    constexpr unsigned phys_sz_bytes = ops_type::aliasing * phys_sz.count();
                    constexpr unsigned logical_alloced_sz = phys_sz_bytes - block_sz;
                    constexpr unsigned phys_alloced_sz = block_sz + phys_sz_bytes;

                    REQUIRE(op.available() == pool_size - phys_alloced_sz);
                    REQUIRE(op.alloced() == logical_alloced_sz);
                }
                SECTION("assess")
                {
                    detail::fragmentation frag;

                    op.assess(&frag);

                    //REQUIRE(frag.candidates[0].is_null() == false);
                }
            }
            SECTION("alloc")
            {
                pool.alloc(handles, 32);
            }
            SECTION("construct")
            {
                int counter = 0;
                int h = pool.construct<SideEffector>(handles, &counter);

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
        constexpr unsigned block_sz = block::header_size<block::Trivial>();

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
