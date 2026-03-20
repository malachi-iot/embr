#include <catch2/catch_all.hpp>

#include <estd/functional.h>
#include <estd/internal/container/traditional_accessor.h>

#include <embr/mem/v1/functional.h>
#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>
#include <embr/mem/v1/vector.h>

#include "test-mem-data.h"

using namespace embr;

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

    for(It i = begin; i < end; ++i)
    {
        ops.get_bundle(*begin);
    }

    mutex.unlock();
}


template <class F, class Pool, Pool* pool = nullptr>
class funclist;

template <class ...Args, class Pool, Pool* pool>
class funclist<void(Args...), Pool, pool> : public embr::mem::v1::vector<mem::detail::sparse_function<void(Args...), Pool>, Pool, pool>
{
    using value_type = mem::detail::sparse_function<void(Args...), Pool>;
    using base_type = embr::mem::v1::vector<value_type, Pool, pool>;
    using handle_type = typename Pool::handle_type;
    using model_type = mem::detail::v1::model<void(Args...), handle_type>;
    using typename base_type::pointer;
    using control_type = typename base_type::impl_type::control_type;
    using base_type::impl;

public:
    template <class ...Args2>
    funclist(Args2&&...args) : base_type(std::forward<Args2>(args)...)  {}

    template <class F>
    friend funclist& operator+=(funclist& self, F&& f)
    {
        Pool* pool2 = self.impl().pool_();
        value_type item(model_type::make(pool2, std::forward<F>(f)));

        self.push_back(item);
        return self;
    }

    void invoke(Args&&...args)
    {
        using impl_type = mem::vector_impl<value_type, Pool, pool>;
        const impl_type& impl = this->impl();
        pointer v = base_type::lock();
        pointer end = v + base_type::size();

        //multi_lock(impl.ops(), v, end, [](auto){});

        for(; v < end; ++v)
        {
            v->invoke(impl.pool_(), std::forward<Args>(args)...);
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
        base_type(model::make(pool2, std::forward<F>(f)), pool2)
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

    // DEBT: This debt lives on, we really need to auto-init the thing
    pool.reset();

    using model_type = mem::detail::v1::model<int(int), handle_type>;
    using fn_type = mem::function<int(int), pool_type>;

    SECTION("basic")
    {
        SECTION("model")
        {
            handle_type h1 = model_type::make(&pool, [](int v) { return v * 2; });

            int r = model_type::invoke(&pool, h1, 5);

            REQUIRE(r == 10);
        }
        SECTION("function")
        {
            fn_type f1(&pool, [](int v) { return v * 2; });
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
    SECTION("funclist")
    {
        int counter = 0;

        funclist<void(int), pool_type> fl(&pool);

        fl += [&](int v) { counter += v * 2; };
        fl += [&](int v) { counter += v; };

        fl.invoke(5);

        REQUIRE(counter == 15);
    }
}
