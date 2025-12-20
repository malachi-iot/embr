#include <catch2/catch_all.hpp>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>


// DEBT: See if we can scoop this from estd
struct SideEffector
{
    int* counter_{};

    SideEffector(int* counter) : counter_{counter}
    {
        ++*counter_;
    }

    ~SideEffector()
    {
        if(*counter_)   --*counter_;
    }
};

using namespace embr::mem;

TEST_CASE("gc mem v1 tests", "[memory][gc]")
{
    SECTION("filter_iterator")
    {
        constexpr int values[] { 0, 4, 7, 9, 3, 6 };

        auto filter = [](int v) { return v > 5; };
        using iterator = v1::filter_iterator<decltype(filter), const int*>;
        iterator i{values};

        REQUIRE(*i == 7);
        ++i;
        REQUIRE(*i == 9);
        ++i;
        REQUIRE(*i == 6);
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
        SECTION("pool: layer1")
        {
            using page = detail::v1::page<uint16_t>;
            using handles_type = detail::v1::handles<detail::v1::handles_traits<page[20]>>;
            using pool_type = detail::v1::pool<detail::v1::pool_traits<char[2048]>>;

            handles_type handles;
            pool_type pool;

            // DEBT: Need to make this automatic
            pool.reset(handles);

            SECTION("ops")
            {

            }
            SECTION("alloc")
            {
                pool.alloc(handles, 32, 8);
            }
            SECTION("construct")
            {
                int counter = 0;
                int h = pool.construct<SideEffector>(handles, &counter);

                //REQUIRE(counter == 1);
            }
        }
    }
}
