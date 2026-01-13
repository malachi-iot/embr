#include <catch2/catch_all.hpp>

#include <estd/functional.h>
#include <estd/internal/container/traditional_accessor.h>

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
    using lock_handle = mem::detail::v1::lock_handle<Pool, pool>;
    using base_type = lock_handle;

protected:
    using function_type = estd::detail::function<R(Args...)>;
    using model_base = typename function_type::model_base;
    using base_type::pool_;
    using typename base_type::handle_type;

public:
    constexpr explicit model(handle_type h = base_type::null, Pool* p = nullptr) : base_type(h, p)   {}

    //model(const model& copy_from) : base_type{copy_from} {}

    template <class F>
    static handle_type make(Pool* pool2, F&& f)
    {
        using model_type = typename function_type::template model<F>;

        return pool2->template construct<model_type>(std::forward<F>(f));
    }

    static R invoke(lock_handle h, Args&&...args)
    {
        //typename base_type::template guard<model_base> g{h};

        auto underlying = (model_base*) h.lock();

        R r = underlying->operator()(std::forward<Args>(args)...);

        h.unlock();

        return r;
    }

    friend R invoke(model& m, Args&&...args)
    {
        Pool* p = m.pool_();
        return model::invoke(m, std::forward<Args>(args)...);
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
        base_type(base_type::make(pool2, std::forward<F>(f)), pool2)
    {

    }


    R operator()(Args&&...args)
    {
        return invoke(*this, std::forward<Args>(args)...);
    }
};

// Heavy lift
template <class T, class Pool, Pool* pool = nullptr>
class vector_impl : public mem::detail::v1::lock_handle<Pool, pool>
{
    using base_type = mem::detail::v1::lock_handle<Pool, pool>;

    int size_{};

public:
    vector_impl(Pool* p) : base_type(base_type::null, p)  {}

    ESTD_CPP_STD_VALUE_TYPE(T)

    struct policy_type
    {

    };

    struct allocator_type
    {
        ESTD_CPP_STD_VALUE_TYPE(T)
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
        //using iterator = pointer;
        using const_iterator = const_pointer;
        using accessor = estd::internal::traditional_accessor<value_type>;
        using iterator = estd::internal::locking_iterator<allocator_type, accessor>;
    };

    ESTD_CPP_CONSTEXPR(17) pointer lock(unsigned pos = 0, unsigned count = 0)
    {
        return ((pointer)base_type::lock()) + pos;
    }

    constexpr unsigned size() const { return size_; }

    int reallocate(unsigned sz)
    {
        return {};
    }
};


template <class T, class Pool, Pool* pool = nullptr>
using vector = estd::internal::dynamic_array<vector_impl<T, Pool, pool>>;

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
    T operator[](int index)
    {
        T val = *lock(index);
        unlock();
        return val;
    }

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
    using model_type = model<R(Args...), Pool, pool>;

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
    using handle_type = pool_type::handle_type;
    using lock_handle = mem::detail::v1::lock_handle<pool_type>;
    pool_type pool;

    // DEBT: This debt lives on, we really need to auto-init the thing
    pool.reset();

    using model_type = model<int(int), pool_type>;
    using fn_type = function<int(int), decltype(pool)>;

    SECTION("basic")
    {
        SECTION("model")
        {
            handle_type h1 = model_type::make(&pool, [](int v) { return v * 2; });
            model_type m1(h1, &pool);

            REQUIRE(m1.has_value());

            int r = model_type::invoke({ h1, &pool }, 5);

            REQUIRE(r == 10);

            model_type m2(std::move(m1));

            r += invoke(m2, 5);

            REQUIRE(r == 20);

            REQUIRE(m1.has_value() == false);
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
        using vector_type = vector<int, pool_type>;

        vector_type vector(&pool);

        //vector.push_back(1);
    }
    SECTION("vector2")
    {
        using vector_type = vector2<int, pool_type>;
        //vector2<int, pool_type> v;
    }
}
