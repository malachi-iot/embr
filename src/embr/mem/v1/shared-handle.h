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
    using handle_type = typename Pool::handle_type;

    handle_type handle_;

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    constexpr lock_handle(handle_type handle, Pool* p) :
        base_type{p},
        handle_{handle}
    {
    }

    void* lock() const
    {
        return value()->lock(handle_);
    }

    void unlock() const
    {
        value()->unlock(handle_);
    }
};

template <class Pool, Pool* pool>
class shared_handle : public lock_handle<Pool, pool>
{
    using base_type = lock_handle<Pool, pool>;
    using base_type::value;
    using base_type::handle_;
    using handle_type = typename Pool::handle_type;

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    shared_handle(handle_type handle, Pool* p) :
        base_type(handle, p)
    {
        value()->ops().ref_up(handle_);
    }

    static constexpr bool global = false;

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

    pointer lock() const { return static_cast<pointer>(base_type::lock()); }
};

}}}
