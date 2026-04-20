#include <catch2/catch_all.hpp>

#include <random>

#include <estd/functional.h>
#include <estd/internal/container/traditional_accessor.h>

#include <embr/mem/v1/functional.h>
#include <embr/mem/v1/functional/list.h>
#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>
#include <embr/mem/v1/vector.h>

#include "test-mem-data.h"

using namespace embr;

template <class Pool>
static void battery(Pool& pool, int it, unsigned seed)
{
    using bytes = mem::bytes_unit<unsigned>;
    using pool_type = Pool;
    using ops_type = typename pool_type::ops_type;
    using bundle = typename ops_type::bundle;
    using type = mem::v1::funclist<void(int), pool_type>;

    ops_type& ops = pool.ops();

    using namespace mem::detail::v1;
    std::mt19937 gen{seed}; // fixed seed: deterministic sequence
    int allocated_handles = count_allocated(ops.handles());

    CAPTURE(it, seed, allocated_handles);

    // FIX: Release mode failure
    VERIFY(allocated_handles == 1);

    {
        int counter1 = 0, counter2 = 0;

        type fl1(&pool);

        fl1 += [&](int)     { ++counter1; };
        fl1 += [&](int v)   { counter1 += v; };

        fl1.invoke(5);

        VERIFY(6);
    }

    allocated_handles = count_allocated(ops.handles());
    VERIFY(allocated_handles == 1);

    {
        int counter1 = 0, counter2 = 0;

        type fl1(&pool), fl2(&pool);

        fl1 += [&](int)     { ++counter1; };
        fl1 += [&](int v)   { counter2 += v; };
        fl2.push_back([&](int) { ++counter1; });
        fl1.invoke(it);
        fl2.invoke(it);

        VERIFY(counter1 == 2);
        VERIFY(counter2 == it);
    }
}

/*
// FIX: enable_if clumsy and incorrect here.  Just whipping it up and easily collides with sparse_function flavor
template <class T, class Pool, Pool* pool>
struct estd::internal::dynamic_array_helper<vector_impl<T, Pool, pool>, estd::enable_if_t<estd::is_same<T, int>::value>>
{

};  */

namespace embr { namespace mem { inline namespace v1 {

// Not loving all this 'pool' repetition
//template <class Pool, Pool* pool, class R, class ...Args>
//class shared_handle<function<detail::v1::lock_handle<Pool, pool>, R(Args...)>, Pool, pool>
//{
//
//};

template <class Pool, Pool* pool, class R, class ...Args>
class shared_handle<estd::detail::function<R(Args...)>, Pool, pool> :
    public detail::shared_handle<Pool, pool>
{
    using base_type = detail::shared_handle<Pool, pool>;
    using function_type = estd::detail::function<R(Args...)>;
    using handle_type = typename Pool::handle_type;
    using model = detail::v1::sparse_model<R(Args...), handle_type>;

public:
    shared_handle(handle_type h, Pool* p) : base_type(h, p) {}

    template <class F>
    shared_handle(Pool* pool2, F&& f) :
        base_type(model::make(pool2, std::forward<F>(f)).handle(), pool2)
    {}

    constexpr R operator()(Args&&...args)
    {
        return model::invoke(base_type::pool_(), base_type::handle_, std::forward<Args>(args)...);
    }
};

}}}

TEST_CASE("gc mem v1 estd::detail::function things", "[memory][gc][function]")
{
    using pool_type = mem::v1::layer1::pool<2048, 8>;
    using ops_type = pool_type::ops_type;
    using handles_type = ops_type::handles_type;
    using bundle = ops_type::bundle;
    using const_bundle = ops_type::const_bundle;
    using page_type = pool_type::page_type;
    using handle_type = pool_type::handle_type;
    using lock_handle = mem::detail::v1::lock_handle<pool_type>;
    using bytes = embr::mem::bytes_unit<unsigned>;
    pool_type pool;
    ops_type& ops = pool.ops();

    using fn_type = mem::function<int(int), pool_type>;
    using model_type = fn_type::sparse_model;
    using fn_virt_type = mem::function<int(int), pool_type, nullptr, estd::detail::impl::function_virtual>;
    using virt_model_type = fn_virt_type::sparse_model;

    static_assert(std::is_base_of<
        estd::internal::rtto_base::virtual_base,
        virt_model_type::function_type::model_base>::value);

    SECTION("basic")
    {
        SECTION("model")
        {
            using function_type = model_type::function_type;

            model_type h1 = model_type::make(&pool, [](int v) { return v * 2; });

            int r = model_type::invoke(&pool, h1.handle(), 5);

            bytes sz = ops.logical_size(ops.get_bundle(h1.handle()));

            // Due to https://github.com/malachi-iot/estdlib/issues/189 even non-capturing lambda
            // takes up a minimal amount of space (thus +aliasing)
            REQUIRE(sz == sizeof(function_type::model_base) + ops_type::aliasing);

            REQUIRE(r == 10);
        }
        SECTION("model (virtual)")
        {
            auto h1 = virt_model_type::make(&pool, [](int v) { return v * 2; });

            int r = virt_model_type::invoke(&pool, h1.handle(), 5);

            bytes sz = ops.logical_size(ops.get_bundle(h1.handle()));

            REQUIRE(sz == sizeof(estd::internal::rtto_base::virtual_base));

            REQUIRE(r == 10);
        }
        SECTION("function")
        {
            fn_type f1(&pool, [](int v) { return v * 2; });
        }
        SECTION("function (virtual)")
        {
            fn_virt_type f1(&pool, [](int v) { return v * 2; });

            int r = f1(5);

            REQUIRE(r == 10);
        }
    }
    SECTION("SideEffector")
    {
        int counter = 0;
        SideEffector se(&counter);

        fn_type f1(&pool, [se2 = std::move(se)](int v) { return v * 2; });

        REQUIRE(counter == 1);

        int r = f1(5);

        REQUIRE(r == 10);

        // Calls destructor of lambda, which calls destruct of se2, decrementing counter
        f1.dealloc();

        REQUIRE(counter == 0);
    }
    SECTION("shared_handle")
    {
        /*
         * Ideal: embr::shared_handle<function<int(int)>, Pool>
         */

        using shared_type = mem::v1::shared_handle<estd::detail::function<int(int)>, pool_type>;

        SECTION("constructed")
        {
            shared_type h1(&pool, [] (int v) { return v * 2; });

            int r = h1(5);

            REQUIRE(r == 10);
        }
        SECTION("make_shared")
        {
            //auto h1 = mem::v1::make_shared<mem::function<int(int)>>(pool, [](int v) { return v * 2; });
        }
    }
    SECTION("funclist: life cycle")
    {
        using type = mem::v1::funclist<void(int), pool_type>;

        auto count = [&] { return count_allocated(ops.handles()); };

        REQUIRE(count() == 1);  // Just free block handle

        {
            type fl(&pool);

            //fl += [](int) {};
        }

        REQUIRE(count() == 1);  // Just free block handle
    }
    SECTION("funclist")
    {
        int counter = 0;
        using type = mem::v1::funclist<void(int), pool_type>;

        type fl(&pool);

        SECTION("nominal")
        {
            fl += [&](int v) { counter += v * 2; };
            fl += [&](int v) { counter += v; };

            fl.invoke(5);

            // FIX: Release mode counter == 0;
            REQUIRE(counter == 15);

            counter = 0;

            auto h = fl.push_back([&](int v) { counter += v; });
            fl.invoke(2);

            REQUIRE(counter == 8);

            fl.erase(h);

            counter = 0;

            fl.invoke(2);

            REQUIRE(counter == 6);
        }
        SECTION("push_back")
        {
            type fl2(&pool);

            fl += [&](int)     { ++counter; };
            fl += [&](int v)   { counter += v; };

            fl2.push_back([&](int) { ++counter; });
        }
    }
    SECTION("funclist: battery")
    {
        std::mt19937 rng{12345}; // NOLINT: fixed seed desired: deterministic sequence

        for(int i = 0; i < 50; ++i)
        {
            pool.ops().reset();
            battery(pool, i, rng());
        }
    }
}
