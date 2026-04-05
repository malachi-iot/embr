#include <catch2/catch_all.hpp>

#include <random>

#include <estd/functional.h>
#include <estd/internal/container/traditional_accessor.h>

#include <embr/mem/v1/functional.h>
#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>
#include <embr/mem/v1/vector.h>

#include "test-mem-data.h"

using namespace embr;

template <class F, class Pool, Pool* pool = nullptr>
class funclist;


template <class Pool>
static void battery(Pool& pool, int it, unsigned seed)
{
    using bytes = mem::bytes_unit<unsigned>;
    using pool_type = Pool;
    using ops_type = typename pool_type::ops_type;
    using bundle = typename ops_type::bundle;
    using type = funclist<void(int), pool_type>;

    ops_type& ops = pool.ops();

    using namespace mem::detail::v1;
    std::mt19937 gen{seed}; // fixed seed: deterministic sequence
    int allocated_handles = count_allocated(ops.handles());

    CAPTURE(it, seed);

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

template <class Traits, class It, class End, class F, class Mutex = internal::noop_mutex>
void multi_lock(const embr::mem::detail::v1::pool_ops<Traits>& ops, It begin, End end, F&& f, Mutex mutex = {})
{
    mutex.lock();

    using handle_type = uint8_t;

    for(It i = begin; i < end; ++i)
    {
        embr::mem::detail::v1::sparse_handle<void, handle_type> sh = *begin;
        //f(sh.lock());
    }

    mutex.unlock();
}

template <class R, class ...Args, class Pool, Pool* pool>
class funclist<R(Args...), Pool, pool>
{
    static_assert(false, "Must have void return signature");
};

template <class ...Args, class Pool, Pool* pool>
class funclist<void(Args...), Pool, pool> :
    protected embr::mem::v1::vector<
        mem::detail::sparse_function<void(Args...), Pool, estd::detail::impl::function_virtual>,
        Pool, pool>
{
    using value_type = mem::detail::sparse_function<void(Args...), Pool, estd::detail::impl::function_virtual>;
    using base_type = embr::mem::v1::vector<value_type, Pool, pool>;
    using handle_type = typename Pool::handle_type;
   // using model_type = typename value_type::model;
    using typename base_type::pointer;
    using control_type = typename base_type::impl_type::control_type;
    using base_type::impl;
    using pinned_type = mem::v1::pinned<base_type>;

    void clear_ll()
    {
        impl().foreach([&](Pool* p, const value_type& f) { f.dealloc(*p); });
    }

public:
    template <class ...Args2>
    constexpr explicit funclist(Args2&&...args) : base_type(std::forward<Args2>(args)...)  {}

    void clear()
    {
        if(impl().is_allocated())
        {
            clear_ll();

            impl().dealloc();
            impl().reset();
        }
    }

    ~funclist()
    {
        if(impl().is_allocated())
            clear_ll();
    }

    template <class F>
    value_type push_back(F&& f)
    {
        value_type item{value_type::make(impl().pool_(), std::forward<F>(f))};
        // FIX: Underlying 'grow by' auto-pads 32, which is OK for the time being but not OK
        // for a fixed default.  See https://github.com/malachi-iot/estdlib/issues/188
        base_type::push_back(item);
        return item;
    }

    // TBD
    void erase(value_type v)
    {
        pinned_type pinned(impl());
        // DEBT: https://github.com/malachi-iot/estdlib/issues/187
        //typename pinned_type::const_iterator it =
        auto it =
            estd::find_if(pinned.begin(), pinned.end(), [v](value_type v2) { return v == v2; });

        if(it == pinned.end())  return;

        pinned.erase(it);
    }

    template <class F>
    friend funclist& operator+=(funclist& self, F&& f)
    {
        self.push_back(std::forward<F>(f));
        return self;
    }

    friend funclist& operator-=(funclist& self, value_type v)
    {
        self.erase(v);
        return self;
    }

    template <class ...Args2>
    void invoke(Args2...args)
    {
        using impl_type = mem::detail::v1::vector<value_type, Pool, pool>;
        const impl_type& impl = this->impl();
        pointer v = base_type::lock();
        pointer end = v + base_type::size();

        multi_lock(impl.ops(), v, end, [](auto){});

        for(; v < end; ++v)
        {
            v->invoke(impl.pool_(), std::forward<Args2>(args)...);
        }

        base_type::unlock();
    }
};

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
    using model = detail::v1::model<R(Args...), handle_type>;

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
    pool_type pool;

    using model_type = mem::detail::v1::model<int(int), handle_type>;
    using fn_type = mem::function<int(int), pool_type>;
    using fn_virt_type = mem::function<int(int), pool_type, nullptr, estd::detail::impl::function_virtual>;

    static_assert(std::is_base_of<
        estd::internal::rtto_base::virtual_base,
        fn_virt_type::model::function_type::model_base>::value);

    SECTION("basic")
    {
        SECTION("model")
        {
            model_type h1 = model_type::make(&pool, [](int v) { return v * 2; });

            int r = model_type::invoke(&pool, h1.handle(), 5);

            REQUIRE(r == 10);
        }
        SECTION("function")
        {
            fn_type f1(&pool, [](int v) { return v * 2; });
        }
        SECTION("function (virtual)")
        {
            fn_virt_type f1(&pool, [](int v) { return v * 2; });

            // FIX: Not working, f1 thinks it's an RttoBase
            //int r = f1(5);

            //REQUIRE(r == 10);
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
        using type = funclist<void(int), pool_type>;

        ops_type& ops = pool.ops();

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
        using type = funclist<void(int), pool_type>;

        type fl(&pool);

        SECTION("nominal")
        {
            fl += [&](int v) { counter += v * 2; };
            fl += [&](int v) { counter += v; };

            fl.invoke(5);

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
