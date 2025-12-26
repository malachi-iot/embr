#pragma once

#include <estd/internal/value_evaporator.h>

#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// Overlap with estd::experimental::global_provider

template <class T, T* t>
struct global_provider<T, t, true>
{
    T* value() const { return t; }

    constexpr global_provider(T*) {}
};

template <class T, T* t>
struct global_provider<T, t, false>
{
    T* t_;

    T* value() const { return t_; }

    constexpr global_provider(T* v) : t_{v}   {}
};

template <class Pool, Pool* pool>
class lock_handle : public global_provider<Pool, pool>
{
protected:
    using base_type = global_provider<Pool, pool>;
    using base_type::value;
    using handles_traits = typename Pool::handles_traits;
    using handle_type = typename Pool::handle_type;
    constexpr static bool is_global = pool != nullptr;

    handle_type handle_;

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    constexpr lock_handle(handle_type handle, Pool* p) :
        base_type{p},
        handle_{handle}
    {
    }

    constexpr lock_handle(handle_type handle) :
        base_type{nullptr},
        handle_{handle}
    {
        static_assert(is_global);
    }

    void* lock() const
    {
        return value()->lock(handle_);
    }

    void unlock() const
    {
        value()->unlock(handle_);
    }

    // EXPERIMENTAL
    void reset()
    {
        handle_ = handles_traits::null;
    }
};


template <class Pool, Pool* pool>
class lock_guard
{
protected:
    using handle = lock_handle<Pool, pool>;

    handle handle_;
    void* data_;       // DEBT: Pull this direct from block

public:
    lock_guard(handle h) : handle_{h},
        data_{h.lock()}
    {
    }

    lock_guard(const lock_guard& copy_from) :
        handle_{copy_from.handle_},
        data_{handle_.lock()}
    {
    }

    lock_guard(lock_guard&& move_from) :
        handle_{move_from.handle_},
        data_{move_from.data_}
    {
        move_from.data_ = nullptr;
    }

    void* data() const { return data_; }

    ~lock_guard()
    {
        // DEBT: Inspect & modify handle_ directly for this
        if(data_)   handle_.unlock();
    }
};

#if __cpp_deduction_guides
template <class Pool, Pool* pool>
lock_guard(shared_handle<Pool, pool>) -> lock_guard<Pool, pool>;
#endif




template <class Pool, Pool* pool>
class shared_handle : public lock_handle<Pool, pool>
{
    using base_type = lock_handle<Pool, pool>;
    using base_type::value;
    using base_type::is_global;
    using base_type::handle_;
    using handle_type = typename Pool::handle_type;

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    shared_handle(handle_type handle, Pool* p) :
        base_type(handle, p)
    {
        value()->ops().ref_up(handle_);
    }

    shared_handle(handle_type handle) :
        base_type(handle)
    {
        value()->ops().ref_up(handle_);
    }

    static constexpr bool global = false;

    lock_guard<Pool, pool> guard() { return { *this }; }

    ~shared_handle()
    {
        value()->ops().ref_down(handle_);
    }
};

}}

inline namespace v1 {

template <class T, class Pool, Pool* pool>
class shared_handle :
    public detail::shared_handle<Pool, pool>
{
    using base_type = detail::shared_handle<Pool, pool>;

public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    shared_handle(int handle, Pool* p = nullptr) : base_type(handle, p) {}

    lock_guard<value_type, Pool, pool> guard() { return { *this }; }

    pointer lock() const { return static_cast<pointer>(base_type::lock()); }
};

template <class T, class Pool, Pool* pool>
class lock_guard : public detail::lock_guard<Pool, pool>
{
    using base_type = detail::lock_guard<Pool, pool>;
    using base_type::data_;
    using typename base_type::handle;

public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    lock_guard(handle h) : base_type(h) {}

    reference operator*() { return *(pointer)data_; }
    constexpr const_reference operator*() const { return *(pointer)data_; }
    pointer operator->() const { return (pointer)data_; }

    pointer data() const { return (pointer)data_; }
};


#if __cpp_deduction_guides
template <class T, class Pool, Pool* pool>
lock_guard(shared_handle<T, Pool, pool>) -> lock_guard<T, Pool, pool>;
#endif


// DEBT: Need to filter this more, otherwise ADL is gonna lose its mind
template <class T, class Pool, class ...Args>
shared_handle<T, Pool, nullptr> make_shared(Pool& pool, Args&&...args)
{
    return { pool.template construct<T>(std::forward<Args>(args)...), &pool };
}

}}}
