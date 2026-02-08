#include <catch2/catch_all.hpp>

#include <estd/functional.h>
#include <estd/internal/container/traditional_accessor.h>

#include <embr/mem/v1/functional.h>
#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"

using namespace embr;


// Heavy lift
template <class T, class Pool, Pool* pool = nullptr>
class vector_impl : public mem::detail::v1::lock_handle<Pool, pool>
{
    using base_type = mem::detail::v1::lock_handle<Pool, pool>;

    int size_{};

public:
    vector_impl(Pool* p) : base_type(base_type::null, p)  {}

    using size_type = unsigned;

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

    constexpr size_type size() const { return size_; }

    int reallocate(unsigned sz)
    {
        return {};
    }

    allocator_type get_allocator() { return {}; }
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
    using handle_type = typename Pool::handle_type;
    using model_type = mem::detail::v1::model<R(Args...), handle_type>;

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
