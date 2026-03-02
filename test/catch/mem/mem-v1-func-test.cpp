#include <catch2/catch_all.hpp>

#include <estd/functional.h>
#include <estd/internal/container/traditional_accessor.h>

#include <embr/mem/v1/functional.h>
#include <embr/mem/v1/pool.hpp>
#include <embr/mem/v1/shared-handle.h>
#include <embr/mem/v1/unique-handle.h>

#include "test-mem-data.h"

using namespace embr;


template <class T, class Pool, Pool* pool = nullptr>
class vector_impl;

namespace detail {

// Effectively alter ego of layer1::vector:
// - estd one is size + inline array + const length
// - this one is size + inline array + semi-const length
// Really similar mechanisms differing primarily in max_size acquisition and of course
// we have to lock here
template <class T, class Size = int>
class vector_impl
{
    template <class T2, class Pool, Pool* pool>
    friend class ::vector_impl;

public:
    using size_type = Size;

    ESTD_CPP_STD_VALUE_TYPE(T)

    T* data() { return reinterpret_cast<T*>(this + 1); }
    const_pointer data() const { return reinterpret_cast<const_pointer>(this + 1); }

    vector_impl() = default;

    vector_impl(const vector_impl& copy_from) :
        size_{copy_from.size_}
    {
        // DEBT: Make an estd flavor of this https://github.com/malachi-iot/estdlib/issues/181
        std::uninitialized_copy_n(copy_from.data(), size_, data());
    }

    // DEBT: If this guy isn't present, rtto incorrectly finds above copy_from during a move request
    vector_impl(vector_impl&& move_from) :
        size_{move_from.size_}
    {
        // DEBT: Make an estd flavor of this https://github.com/malachi-iot/estdlib/issues/181
        std::uninitialized_move_n(move_from.data(), size_, data());
    }

private:
    size_type size_{};

public:

    constexpr size_type size() const { return size_; }
};


}


// Heavy lift
// Not done, I'd say over the hump for a proof of concept
template <class T, class Pool, Pool* pool>
class vector_impl : public mem::v1::unique_handle<detail::vector_impl<T>, Pool, pool>
{
    using control_type = detail::vector_impl<T>;
    using base_type = mem::v1::unique_handle<control_type, Pool, pool>;
    using this_type = vector_impl;
    using typename base_type::ops_type;
    using pos_type = typename ops_type::pos_type;
    using bundle = typename ops_type::bundle;
    using const_bundle = typename ops_type::const_bundle;
    using base_type::ops;
    using base_type::guard;
    using bytes = estd::units::bytes<unsigned>;

    using base_type::handle_;

    static constexpr unsigned control_size = sizeof(control_type);

    //static_assert(control_size == sizeof(void*));

    template <class ...Args>
    bundle construct_ll(int reserved, Args&&...args)
    {
        using block = embr::mem::detail::block_8;

        constexpr block::modes mode = embr::mem::detail::ascertain_block_mode<control_type>();
        constexpr bytes block_sz = block::header_size(mode);
        unsigned sz = reserved * sizeof(T) + block_sz.count() + sizeof(control_type);

        // construct_ll takes explicit size as 1st parameter as you might glean
        return ops().template construct_ll<mode, control_type>(ops().do_alias(sz), std::forward<Args>(args)...);
    }

public:
    using base_type::pool_;

    vector_impl(Pool* p) : base_type(base_type::null, p)  {}

    vector_impl(const vector_impl& copy_from) :
        base_type(base_type::null, copy_from.pool_())
    {
        if(!copy_from.has_value())  return;

        const control_type* c = copy_from.clock();

        // DEBT: Consider overprovisioning
        handle_ = construct_ll(c->size_, *c).handle;

        copy_from.unlock();
    }

    vector_impl(vector_impl&& move_from) :
        base_type(move_from.handle_, move_from.pool_())
    {
        move_from.handle_ = base_type::null;
    }

    using size_type = unsigned;

    ESTD_CPP_STD_VALUE_TYPE(T)

    struct policy_type
    {

    };

    struct handle_with_offset_old
    {
    };

    using handle_with_offset = estd::internal::handle_with_offset_raw<pointer>;

    struct allocator_type
    {
        using size_type = unsigned;

        ESTD_CPP_STD_VALUE_TYPE(T)

        Pool* pool_;

        // DEBT: Document why we need this up in estd
        using const_void_pointer = const void*;
        using handle_with_offset = typename this_type::handle_with_offset;

        reference lock(const handle_with_offset& h) { return *h.handle(); }
    };

    struct allocator_traits
    {
        ESTD_CPP_STD_VALUE_TYPE(T)
        using size_type = unsigned;
        using handle_type = mem::detail::v1::lock_handle<Pool, pool>;

        // FIX: One or multiple of these are wanting to be an accessor
        using allocator_valref = allocator_type;
        //using iterator = pointer;
        using const_iterator = const_pointer;
        using accessor = estd::internal::traditional_accessor<value_type>;
        using iterator = estd::internal::locking_iterator<allocator_type, accessor>;
        using handle_with_offset = typename this_type::handle_with_offset;

        static constexpr auto locking_preference = estd::internal::allocator_locking_preference::standard;
    };

    handle_with_offset offset(unsigned pos) const
    {
        // FIX: We need the non-pointer variety of handle_with_offset
        // FIX: Feed this non-nullptr
        // FIX: We need to not use this temporal pointer
        control_type* control = base_type::lock();
        base_type::unlock();

        return { control->data() + pos };
    }

    ESTD_CPP_CONSTEXPR(17) reference lock(unsigned pos = 0, unsigned count = 0)
    {
        control_type* control = base_type::lock();

        // DEBT: lock return a reference is somewhat counterintuitive
        return *(control->data() + pos);
    }

    size_type size() const
    {
        if(!is_allocated()) return 0;

        return guard()->size_;
    }

    size_type max_size() const
    {
        // This involves asking pool/ops for available contiguous free space.  Don't do fancy
        // theoretical/defrag calc here - let someone else do defragging first before calling max_size()
        return {};  // TBD
    }

    unsigned capacity() const
    {
        if(is_allocated() == false) return 0;

        // DEBT: assert (in debug mode only) that we're evenly divisible
        const bytes size = base_type::logical_size();
        return (size.count() - control_size) / sizeof(T);
    }

    constexpr bool is_allocated() const { return base_type::has_value(); }

    void size(unsigned new_size)
    {
        /*
        bytes rsz(new_size * sizeof(T));
        bytes sz = ops().phys_size(get_bundle());

        sz -= control_size;

        // DEBT: Consider padding here
        if(rsz > sz)    assert(reallocate(new_size));   */
        guard()->size_ = new_size;
    }

    bool reallocate(unsigned capacity)
    {
        // DEBT: Check estd, it may be that reallocate is NEVER called in this unallocated
        // condition.  Leaning strongly towards it handles that for us
        if(is_allocated() == false) return allocate(capacity);

        bool success = pool_()->realloc(handle_, control_size + capacity * sizeof(T), &handle_);
        return success;
    }

    allocator_type get_allocator() { return { pool_() }; }

    bool allocate(unsigned capacity)
    {
        assert(!is_allocated());

        handle_ = construct_ll(capacity).handle;

        return is_allocated();
    }

    // DEBT: Seems superfluous - like estd could do all this on our behalf
    template <class ...Args>
    void construct(int pos, Args&&...args)
    {
        control_type* control = base_type::lock();

        new (control->data() + pos) value_type(std::forward<Args>(args)...);

        base_type::unlock();
    }
};

/*
// FIX: enable_if clumsy and incorrect here.  Just whipping it up and easily collides with sparse_function flavor
template <class T, class Pool, Pool* pool>
struct estd::internal::dynamic_array_helper<vector_impl<T, Pool, pool>, estd::enable_if_t<estd::is_same<T, int>::value>>
{

};  */


template <class T, class Pool, Pool* pool = nullptr>
using vector = estd::internal::dynamic_array<vector_impl<T, Pool, pool>>;

template <class T, class Pool, Pool* pool = nullptr>
class vector_revealed : public vector<T, Pool, pool>
{
    using base_type = vector<T, Pool, pool>;

public:
    using base_type::impl;

    template <class ...Args>
    constexpr vector_revealed(Args&&...args) :
        base_type(std::forward<Args>(args)...) {}
};

// DEBT: Put this guy up in estd
namespace mixins {
// Span-esque
template <class Derived, class T>
class container
{
public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    reference operator[](int v)
    {
        return *(static_cast<Derived*>(this)->data() + v);
    }

    constexpr const_reference operator[](int v) const
    {
        return *(static_cast<const Derived*>(this)->data() + v);
    }

    pointer begin()
    {
        return static_cast<Derived*>(this)->data();
    }

    constexpr const_pointer cbegin() const
    {
        return static_cast<const Derived*>(this)->data();
    }

    constexpr const_pointer begin() const { return cbegin(); }

    pointer end()
    {
        auto self = static_cast<Derived*>(this);
        return self->data() + self->size();
    }

    const_pointer cend() const
    {
        auto self = static_cast<const Derived*>(this);
        return self->data() + self->size();
    }

    const_pointer end() const { return cend(); }

    const_reference front()
    {
        return *begin();
    }

    constexpr const_reference front() const
    {
        return *begin();
    }

    constexpr bool empty() const
    {
        return static_cast<const Derived*>(this)->size() == 0;
    }
};

}

// EXPERIMENTAL, probably disambiguate with a name like 'pinned' since this behaves slightly
// differently than lock_guard (that's an has-a wrapper, this is a sort of an is-a reinterpreter)
template <class T, class Pool, Pool* pool>
class embr::mem::v1::lock_guard<vector<T, Pool, pool>, void, nullptr> :
    public embr::mem::v1::lock_guard<::detail::vector_impl<T>, Pool, pool>,
    public mixins::container<embr::mem::v1::lock_guard<vector<T, Pool, pool>, void, nullptr>, T>
{
    using vector_type = ::detail::vector_impl<T>;
    using base_type = embr::mem::v1::lock_guard<vector_type, Pool, pool>;

public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    using size_type = typename vector_type::size_type;

    pointer data() { return base_type::data()->data(); }
    constexpr const_pointer data() const { return base_type::data()->data(); }

    template <class ...Args>
    constexpr lock_guard(Args&&...args) : base_type(std::forward<Args>(args)...) {}

    constexpr size_type size() const { return base_type::data()->size(); }
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

    constexpr int size() const { return size_; }
};


template <class F, class Pool, Pool* pool = nullptr>
class funclist;

template <class ...Args, class Pool, Pool* pool>
class funclist<void(Args...), Pool, pool> : public vector<mem::detail::sparse_function<void(Args...), Pool>, Pool, pool>
{
    using value_type = mem::detail::sparse_function<void(Args...), Pool>;
    using base_type = vector<value_type, Pool, pool>;
    using handle_type = typename Pool::handle_type;
    using model_type = mem::detail::v1::model<void(Args...), handle_type>;
    using typename base_type::pointer;

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

    void operator()(Args&&...args)
    {
        // TBD do mutex here
        pointer v = base_type::lock();
        pointer end = v + base_type::size();

        for(; v < end; ++v)
        {
            v->invoke(base_type::pool_(), std::forward<Args>(args)...);
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
    SECTION("vector_impl")
    {
        using type = detail::vector_impl<char>;
        type vi;

        REQUIRE(vi.data() == ((char*)&vi) + sizeof(type));
    }
    SECTION("vector: int")
    {
        using vector_type = vector<int, pool_type>;
        const handles_type& handles = pool.ops().handles();

        handles_type::const_iterator it = handles.cbegin();

        // Always 1 'free' handle, skip him
        bool valid = ++it == handles.end();
        REQUIRE(valid);

        vector_type vector(&pool);
        const auto& revealed = (vector_revealed<int, pool_type>&)vector;

        vector.push_back(1);
        vector.push_back(2);
        REQUIRE(pool.ops_.invariant());

        // After first push, that's when we actually allocate.  Grab and make sure
        it = handles.begin();
        // NOTE: Be aware that we can't copy this guy because get_bundle secretly takes address.  This is acceptable
        // because ops() are low level in nature and even passing in a pointer doesn't clear this up much
        const page_type& page = *it;
        const_bundle bn = pool.ops().get_bundle(page);
        const_bundle bn2 = revealed.impl().get_bundle();

        // DEBT: Ultimately displace with 'revealed' approach
        REQUIRE(bn.handle == bn2.handle);
        REQUIRE(bn.allocated());
        // DEBT: Fine tune vector padding/reservation code so that this is more predictable
        REQUIRE(pool.ops().phys_size(bn).count() == 8);

        bn = pool.ops().get_bundle(*++it);
        REQUIRE(bn.allocated() == false);
        valid = ++it == handles.end();
        // DEBT: Catch2 is unhappy if we do this particular expression in a REQUIRE
        REQUIRE(valid);

        REQUIRE(vector.size() == 2);
        REQUIRE(vector.at(0) == 1);
        REQUIRE(*vector.lock() == 1);
        vector.unlock();

        // This guy will be a true 'grow' (no memory moved)
        vector.reserve(3);

        // Indeed no memory movement occurs
        REQUIRE(handles.begin()->pos() == page.pos());

        vector_type vector1(&pool);

        // He doesn't allocate from pool until we push something, so do so
        vector1.push_back(1);

        // This guy will require memory movement
        vector.reserve(20);

        bn = revealed.impl().get_bundle();

        REQUIRE(bn.pos() != page.pos());
        // We skip relinking handle, so double check we have indeed moved to a new handle#
        REQUIRE(bn2.handle != bn.handle);
        REQUIRE(bn.handle == 2);

        vector_type copied(vector);
        const auto& copied_revealed = (vector_revealed<int, pool_type>&)copied;

        const_bundle bn_copied = copied_revealed.impl().get_bundle();

        // Remember 0 was freed up when doing vector.reserve, so it is reused for the new copied
        // vector
        REQUIRE(bn_copied.handle == 0);
        //REQUIRE(copied == vector);
    }
    SECTION("vector: SideEffector")
    {
        int counter = 0;

        using vector_type = vector<SideEffector, pool_type>;

        vector_type vector(&pool);
        std::vector<SideEffector> parity;

        vector.push_back({});

        // Calls default ctor, then move ctor
        parity.push_back({});

        // FIX: A dangling lock should cause an assert, but our iterator/handle treatment is
        // still goofy
        REQUIRE(vector[0].clock().moved_to_counter == 1);
        REQUIRE(parity[0].moved_to_counter == 1);

        vector.emplace_back(&counter);

        REQUIRE(vector[1].clock().counter() == 1);
        REQUIRE(vector[1].clock().moved_to_counter == 0);
    }
    SECTION("vector: lock_guard (pinned) - EXPERIMENTAL")
    {
        using vector_type = vector<int, pool_type>;
        vector_type vector(&pool);
        auto& revealed = (vector_revealed<int, pool_type>&) vector;

        vector.push_back(5);
        vector.push_back(10);

        mem::v1::lock_guard<vector_type> pinned(revealed.impl());

        REQUIRE(pinned[0] == 5);

        const int* i = pinned.begin();

        REQUIRE(*i++ == 5);
        REQUIRE(*i == 10);

        REQUIRE(pinned.empty() == false);
    }
    SECTION("vector2")
    {
        using vector_type = vector2<int, pool_type>;
        //vector2<int, pool_type> v;
    }
    SECTION("funclist")
    {
        funclist<void(int), pool_type> fl(&pool);

        fl += [](int v) {};
    }
}
