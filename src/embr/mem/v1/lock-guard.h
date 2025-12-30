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
};

template <class T, T* t>
struct global_provider<T, t, false>
{
    T* t_;

    T* value() const { return t_; }

    constexpr explicit global_provider(T* v) : t_{v}   {}
};

template <class Derived>
class lock_handle_crtp
{
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
    constexpr static handle_type null = handles_traits::null;

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

template <class Pool, Pool* pool>
class lock_guard
{
protected:
    using handle = lock_handle<Pool, pool>;

    handle handle_;
    void* data_;       // DEBT: Pull this direct from block

public:
    constexpr explicit lock_guard(handle h) : handle_{h},
        data_{h.lock()}
    {
    }

    constexpr lock_guard(const lock_guard& copy_from) :
        handle_{copy_from.handle_},
        data_{handle_.lock()}
    {
    }

    constexpr lock_guard(lock_guard&& move_from) noexcept :
        handle_{std::move(move_from.handle_)},
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

}}

inline namespace v1 {

template <class T, class Pool, Pool* pool>
class lock_guard : public detail::lock_guard<Pool, pool>
{
    using base_type = detail::lock_guard<Pool, pool>;
    using base_type::data_;
    using typename base_type::handle;

public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    template <class ...Args>
    constexpr lock_guard(Args&&...args) :
        base_type(std::forward<Args>(args)...) {}

    reference operator*() { return *(pointer)data_; }
    constexpr const_reference operator*() const { return *(pointer)data_; }
    pointer operator->() const { return (pointer)data_; }

    pointer data() const { return (pointer)data_; }
};

}

}}
