#include <catch2/catch_all.hpp>

#include <estd/functional.h>

#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"

using namespace embr;

template <class F, class Pool, Pool* pool = nullptr>
class model;

template <class F, class Pool, Pool* pool = nullptr>
class function;


// DEBT: Consider making model just track the handle, letting 'function' be the one who tracks pool too
template <class R, class ...Args, class Pool, Pool* pool>
class model<R(Args...), Pool, pool> :
    public mem::detail::v1::lock_handle<Pool, pool>
{
    using base_type = mem::detail::v1::lock_handle<Pool, pool>;

protected:
    using function_type = estd::detail::function<R(Args...)>;
    using model_base = typename function_type::model_base;
    using typename base_type::handle_type;

public:
    constexpr explicit model(handle_type h = base_type::null, Pool* p = nullptr) : base_type(h, p)   {}

    //model(const model& copy_from) : base_type{copy_from} {}

    template <class F>
    static model make(Pool* pool2, F&& f)
    {
        using model_type = typename function_type::template model<F>;

        typename base_type::handle_type h = pool2->template construct<model_type>(std::forward<F>(f));

        // DEBT: As a low level method, we should return only the handle
        return model{ h, pool2 };
    }

    R operator()(Args&&...args)
    {
        auto underlying = (model_base*) base_type::lock();

        R r = underlying->operator()(std::forward<Args>(args)...);

        base_type::unlock();

        return r;
    }
};


template <class R, class ...Args, class Pool, Pool* pool>
class function<R(Args...), Pool, pool> :
    public model<R(Args...), Pool, pool>
{
    using base_type = model<R(Args...), Pool, pool>;
    using typename base_type::model_base;

public:
    function(estd::nullptr_t) {}

    template <class F>
    function(Pool* pool2, F&& f) :
        base_type(base_type::make(pool2, std::forward<F>(f)))
    {

    }


    R operator()(Args&&...args)
    {
        auto underlying = (model_base*) base_type::lock();

        R r = underlying->operator()(std::forward<Args>(args)...);

        base_type::unlock();

        return r;
    }
};

// Heavy lift
template <class T, class Pool, Pool* pool = nullptr>
class vector_impl : public mem::detail::v1::lock_handle<Pool, pool>
{
    using base_type = mem::detail::v1::lock_handle<Pool, pool>;

public:
    vector_impl() : base_type(base_type::null)  {}

    struct policy_type
    {

    };

    struct allocator_type
    {

    };

    struct allocator_traits
    {
        ESTD_CPP_STD_VALUE_TYPE(T)
        using size_type = unsigned;
        using handle_type = mem::detail::v1::lock_handle<Pool, pool>;

        struct handle_with_offset
        {
        };

        // FIX: One or multiple of these are wanting to be an accessor
        using allocator_valref = int;
        using iterator = pointer;
        using const_iterator = const_pointer;
    };

    ESTD_CPP_STD_VALUE_TYPE(T)

    ESTD_CPP_CONSTEXPR(17) pointer lock(unsigned pos = 0, unsigned count = 0)
    {
        return ((pointer)base_type::lock()) + pos;
    }
};


template <class T, class Pool, Pool* pool = nullptr>
class vector : public estd::internal::dynamic_array<vector_impl<T, Pool, pool>>
{
    using base_type = estd::internal::dynamic_array<vector_impl<T, Pool, pool>>;

public:

};

// Because "true" vector is a very heavy lift, creating a cut-down easy mode one
template <class T, class Pool, Pool* pool = nullptr>
class vector2 : public vector_impl<T, Pool, pool>
{
    using base_type = vector_impl<T, Pool, pool>;
    using base_type::lock;
    using base_type::unlock;
    using base_type::pool_;
    using typename base_type::pointer;

    int size_;

public:
    void push_back(const T& value)
    {
        pointer data = lock();

        unlock();
    }

};


template <class F, class Pool, Pool* pool = nullptr>
class funclist;

template <class R, class ...Args, class Pool, Pool* pool>
class funclist<R(Args...), Pool, pool> : public vector<int, Pool, pool>
{
    using base_type = vector<int, Pool, pool>;

public:

    template <class F>
    friend int operator+=(funclist& self, F&& f)
    {
        base_type::push_back(0);
        return 0;
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
    using pool_type = mem::v1::layer1::pool<2048, 8>;
    pool_type pool;

    // DEBT: This debt lives on, we really need to auto-init the thing
    pool.reset();

    using model_type = model<int(int), decltype(pool)>;
    using fn_type = function<int(int), decltype(pool)>;

    SECTION("basic")
    {
        SECTION("model")
        {
            model_type h1 = model_type::make(&pool, [](int v) { return v * 2; });

            REQUIRE(h1);

            int r = h1(5);

            REQUIRE(r == 10);

            model_type h2(std::move(h1));

            r += h2(5);

            REQUIRE(r == 20);

            REQUIRE(h1.has_value() == false);
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

        using shared_type = mem::v1::shared_handle<estd::detail::function<int(int)>, decltype(pool)>;

        shared_type h1 = shared_type::make_handle(&pool, [] (int v) { return v * 2; });

        int r = h1(5);

        REQUIRE(r == 10);
    }
    SECTION("vector")
    {
        //vector2<int, pool_type> v;
    }
}
