#pragma once

#include <estd/units.h>

#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

namespace mixin {

// Maybe overdoing things?
template <class Derived, class T>
struct typed_handle
{
    ESTD_CPP_STD_VALUE_TYPE(T)

    template <class Pool>
    pointer lock() const
    {
        auto self = static_cast<Derived*>(this);
        return static_cast<pointer>(self->lock());
    }
};

}

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

// AKA "sparse handle"
// Not owning
// Not lock-guarded
// Primarily a type-safety mechanism for T
// DEBT: Perhaps actually call this 'sparse_handle' for real
template <class T, class Handle>
class typed_handle
{
public:
    using handle_type = Handle;

protected:
    handle_type handle_;

public:
    constexpr explicit typed_handle(handle_type handle) : handle_{handle}   {}

    template <class Pool>
    T* lock(Pool& pool) const
    {
        return static_cast<T*>(pool.lock(handle_));
    }

    template <class Pool>
    void unlock(Pool& pool) const
    {
        return pool.unlock(handle_);
    }

    template <class Pool>
    void destroy(Pool& pool) const
    {
        pool.destroy(handle_);
    }

    template <class Pool>
    constexpr lock_handle<Pool> handle(Pool& pool) const
    {
        return lock_handle<Pool>(handle_, &pool);
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

    using ops_type = typename Pool::ops_type;
    using pos_type = typename ops_type::pos_type;

    using bundle = typename ops_type::bundle;
    using const_bundle = typename ops_type::const_bundle;

protected:

    handle_type handle_;

    // DEBT: Swap this and template value parameter name
    Pool* pool_() const { return value(); }
    ops_type& ops() { return value()->ops(); }
    constexpr const ops_type& ops() const { return value()->ops(); }

    bundle get_bundle() { return ops().get_bundle(handle_); }

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    constexpr lock_handle(handle_type handle, Pool* p) :
        base_type{p},
        handle_{handle}
    {
        // DEBT: Seems we can do a static_assert for is_global and demand p always be set, depending
        // on nature of those calling us
        assert(p || is_global);
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

    void dealloc() const
    {
        value()->dealloc(handle_);
    }

    constexpr const_bundle get_bundle() const { return ops().get_bundle(handle_); }

    estd::units::bytes<unsigned> logical_size() const
    {
        return ops().logical_size(get_bundle());
    }

    pos_type phys_size() const
    {
        return ops().phys_size(get_bundle());
    }

    constexpr handle_type handle() const { return handle_; }

    explicit constexpr operator bool() const { return handle_ != null; }

    constexpr bool has_value() const { return handle_ != null; }

    // shared_ptr style behavior is tracked at the block level, so expose this here
    // even for parties who aren't using the ref counter
    int use_count() const
    {
        // TBD
        return {};
    }

    void reset()
    {
        handle_ = null;
    }

    // EXPERIMENTAL
    template <class T>
    using guard_type = lock_guard<T, Pool, pool>;

    template <class T = void>
    guard_type<T*> guard() const
    {
        return { *this };
    }
};

// Dummy lock handle just to help out make_shared
template <>
class lock_handle<void, nullptr>
{
public:
    // Never allowed to actually construct
    template <class ...Args>
    lock_handle(Args&&...args) = delete;

    using handle_type = int;
    using ops = void;

    int handle_;

    constexpr int value() { return {}; }
    constexpr void* pool_() { return {}; }
    static constexpr bool is_global = false;
};
    
}}

}}
