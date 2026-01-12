#include <catch2/catch_all.hpp>

#include <estd/functional.h>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"

using namespace embr;

template <class H, class F>
class function;

template <class Pool, Pool* pool, class R, class ...Args>
class function<mem::detail::v1::lock_handle<Pool, pool>, R(Args...)>
{
    using handle_type = mem::detail::v1::lock_handle<Pool, pool>;
    using underlying_type = estd::detail::function<R(Args...)>;
    using model_base = typename underlying_type::model_base;

    handle_type handle_;

public:
    function(handle_type handle) : handle_{handle} {}

    template <class F>
    static handle_type make_handle(Pool* pool2, F&& f)
    {
        using model_type = typename underlying_type::template model<F>;

        typename handle_type::handle_type h = pool2->template construct<model_type>(std::forward<F>(f));

        return { h, pool2 };
    }

    R operator()(Args&&...args)
    {
        auto underlying = (model_base*) handle_.lock();

        R r = underlying->operator()(std::forward<Args>(args)...);

        handle_.unlock();

        return r;
    }
};


namespace embr { namespace mem { inline namespace v1 {

// Not loving all this 'pool' repetition
template <class Pool, Pool* pool, class R, class ...Args>
class shared_handle<function<detail::v1::lock_handle<Pool, pool>, R(Args...)>, Pool, pool>
{

};

template <class Pool, Pool* pool, class R, class ...Args>
class shared_handle<estd::detail::function<R(Args...)>, Pool, pool> :
    public detail::shared_handle<Pool, pool>
{
    using base_type = detail::shared_handle<Pool, pool>;
    using function_type = estd::detail::function<R(Args...)>;
    using model_base = typename function_type::model_base;

public:
    shared_handle(int h, Pool* p) : base_type(h, p) {}

    template <class F>
    static shared_handle make_handle(Pool* pool2, F&& f)
    {
        using model_type = typename function_type::template model<F>;

        int h = pool2->template construct<model_type>(std::forward<F>(f));

        return { h, pool2 };
    }

    R operator()(Args&&...args)
    {
        lock_guard<model_base, Pool, pool> l{*this};

        return l->operator()(std::forward<Args>(args)...);
    }
};

}}}

TEST_CASE("gc mem v1 estd::detail::function things", "[memory][gc][function]")
{
    mem::v1::layer1::pool<2048, 8> pool;

    // DEBT: This debt lives on, we really need to auto-init the thing
    pool.reset();

    using handle_type = mem::detail::v1::lock_handle<decltype(pool)>;

    using fn_type = function<handle_type, int(int)>;

    SECTION("basic")
    {
        handle_type h1 = fn_type::make_handle(&pool, [](int v) { return v * 2; });

        REQUIRE(h1);

        fn_type f1(h1);

        int r = f1(5);

        REQUIRE(r == 10);
    }
    SECTION("SideEffector")
    {
        int counter = 0;
        SideEffector se(&counter);

        handle_type h1 = fn_type::make_handle(&pool, [se2 = std::move(se)](int v) { return v * 2; });

        REQUIRE(counter == 1);

        fn_type f1(h1);

        int r = f1(5);

        REQUIRE(r == 10);

        // Calls destructor of lambda, which calls destruct of se2, decrementing counter
        h1.dealloc();

        REQUIRE(counter == 0);
    }
    SECTION("shared_handle")
    {
        /*
         * Ideal: embr::shared_handle<function<int(int)>, Pool>
         */

        using shared_type = mem::v1::shared_handle<estd::detail::function<int(int)>, decltype(pool)>;

        shared_type h1 = shared_type::make_handle(&pool, [] (int v) { return v * 2; });

        int r = h1(5);

        REQUIRE(r == 10);
    }
}
