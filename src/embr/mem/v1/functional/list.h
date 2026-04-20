#pragma once

#include "../fwd.h"
#include "../functional.h"
#include "../vector.h"

#include "fwd.h"

namespace embr { namespace mem { inline namespace v1 {

// DEBT: I like proactively error'ing out with this static_assert.  However, somehow github actions (probably Clang)
// stokes this into asserting anyway.  Bring this back somehow (probably static assert down within full implementation)
// bringing back R when R is_void
#if UNUSED
template <class R, class ...Args, class Pool, Pool* pool>
class funclist<R(Args...), Pool, pool>
{
    static_assert(false, "Must have void return signature");
};
#endif

// mimics what Boost::signals2::scoped_connection does
// DEBT: isolate in a signals namespace or similar
template <class Pool, Pool* pool>
class connection
{
protected:
    using handle_type = typename Pool::handle_type;

    detail::lock_handle<Pool, pool> list_handle_;
    detail::sparse_handle<void, handle_type> func_handle_;

    using control_type = detail::vector_control<handle_type>;
    using iterator = typename control_type::const_iterator;

    control_type* lock()
    {
        return (control_type*) list_handle_.lock();
    }

    void unlock() { list_handle_.unlock(); }

public:
    // NOT READY YET
    // Likely doesn't compile
    // Idea is to opportunistically cast to control_type, presuming sparse_function is trivially 1:1 with handle_type.
    // With that we then do a trivial erase on the funclist control vector, then manually destroy func_handle - in
    // essence treating him like a unique_ptr which is correct being that funclist is the owner of the functions in it
    // TODO: Put a static_assert somewhere to ensure above presumption
    void remove()
    {
        control_type* control = lock();

        iterator found = estd::find_if(
            control->begin(), control->end(),
            [&](iterator it) { return *it == func_handle_.handle(); });

        if(found != control->end())
        {
            control->erase(found);
            func_handle_.destroy(list_handle_.pool_());
        }

        unlock();
    }
};

// mimics what Boost::signals2::scoped_connection does
// DEBT: isolate in a signals namespace or similar
template <class Pool, Pool* pool>
class scoped_connection : public connection<Pool, pool>
{
    using base_type = connection<Pool, pool>;

public:
};

template <class ...Args, class Pool, Pool* pool>
class funclist<void(Args...), Pool, pool> :
    protected vector<
        detail::v1::sparse_function<void(Args...), Pool, estd::detail::impl::function_virtual>,
        Pool, pool>
{
    using value_type = detail::v1::sparse_function<void(Args...), Pool, estd::detail::impl::function_virtual>;
    using base_type = vector<value_type, Pool, pool>;
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
        impl_type& impl = this->impl();
        pointer begin = base_type::lock(), v = begin;
        pointer end = v + base_type::size();

        multi_lock(impl.ops(), begin, end, impl.pool_()->mutex());

        for(; v < end; ++v)
        {
            v->invoke(impl.pool_(), std::forward<Args2>(args)...);
        }

        multi_unlock(impl.ops(), begin, end, impl.pool_()->mutex());

        base_type::unlock();
    }
};


// DEBT: Doesn't belong in list.h here
template <class It, class End, class F, class Mutex = embr::internal::noop_mutex>
void multi_op(It begin, End end, F&& f, Mutex mutex = {})
{
    mutex.lock();

    for(It i = begin; i < end; ++i) f(*i);

    mutex.unlock();
}

// DEBT: It and End subject to easy errors
template <class Traits, class It, class End, class Mutex>
void multi_lock(detail::v1::pool_ops<Traits>& ops, It begin, End end, Mutex mutex)
{
    using handle_type = typename Traits::handles_type::handle_type;
    using type = detail::v1::sparse_handle<void, handle_type>;

    multi_op(begin, end, [&](type v)
    {
        ops.lock(v.handle());
    }, mutex);
}


// DEBT: It and End subject to easy errors
template <class Traits, class It, class End, class Mutex>
void multi_unlock(detail::v1::pool_ops<Traits>& ops, It begin, End end, Mutex mutex)
{
    using handle_type = typename Traits::handles_type::handle_type;
    using type = detail::v1::sparse_handle<void, handle_type>;

    multi_op(begin, end, [&](type v)
    {
        ops.unlock(v.handle());
    }, mutex);
}


}}} // embr::mem::inline v1
