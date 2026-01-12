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

TEST_CASE("gc mem v1 estd::detail::function things", "[memory][gc][function]")
{
    mem::v1::layer1::pool<2048, 8> pool;

    // DEBT: This debt lives on, we really need to auto-init the thing
    pool.reset();

    using handle_type = mem::detail::v1::lock_handle<decltype(pool)>;

    using fn_type = function<handle_type, int(int)>;

    handle_type h1 = fn_type::make_handle(&pool, [](int v) { return v * 2; });

    REQUIRE(h1);

    fn_type f1(h1);

    int r = f1(5);

    REQUIRE(r == 10);
}
