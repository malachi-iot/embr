#pragma once

#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// Overlap with estd::experimental::global_provider

template <class T, T* t>
struct global_provider<T, t, true>
{
    static ESTD_CPP_CONSTEVAL T* value() { return t; }

    constexpr explicit global_provider(T*) {}

    static constexpr T* global_val = t;
    static constexpr bool is_global = true;
};

template <class T, T* t>
struct global_provider<T, t, false>
{
    T* t_;

    T* value() const { return t_; }

    constexpr explicit global_provider(T* v) : t_{v}   {}

    static constexpr T* global_val = nullptr;
    static constexpr bool is_global = false;
};

template <class Derived>
class lock_handle_crtp
{
public:
    typename Derived::pointer lock()
    {
        auto self = static_cast<Derived*>(this);
        return (typename Derived::pointer)self->lock();
    }

    lock_guard<typename Derived::value_type, typename Derived::pool_type, Derived::global_val> operator()()
    {
        auto self = static_cast<Derived*>(this);
        return { *self };
    }
};

template <class Pool, Pool* pool>
class lock_handle : public global_provider<Pool, pool>
{
protected:
    using base_type = global_provider<Pool, pool>;
    using base_type::value;

public:
    using handles_traits = typename Pool::handles_traits;
    using handle_type = typename Pool::handle_type;
    constexpr static bool is_global = pool != nullptr;
    constexpr static handle_type null = handles_traits::null;

protected:

    handle_type handle_;

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    constexpr lock_handle(handle_type handle, Pool* p) :
        base_type{p},
        handle_{handle}
    {
    }

    constexpr explicit lock_handle(handle_type handle) :
        base_type{nullptr},
        handle_{handle}
    {
        static_assert(is_global);
    }

    constexpr lock_handle(lock_handle&& move_from) noexcept :
        base_type{move_from.value()},
        handle_{move_from.handle_}
    {
        move_from.reset();
    }

    constexpr lock_handle(const lock_handle&) = default;

    void* lock() const
    {
        return value()->lock(handle_);
    }

    void unlock() const
    {
        value()->unlock(handle_);
    }

    explicit constexpr operator bool() const { return handle_ != null; }

    void reset()
    {
        handle_ = null;
    }
};
    
}}

}}
