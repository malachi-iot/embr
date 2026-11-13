#pragma once

#include <estd/type_traits.h>

#include "pool/lock-handle.h"
#include "pool/fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

namespace mixin {

// TODO: Put mixins up into mixins.h

// Maybe overdoing things?
template <class Derived, class T>
struct sparse_handle
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

// Auto-casts to Derived::pointer
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

template <class Handle>
class sparse_handle<void, Handle>
{
public:
    using handle_type = Handle;

protected:
    handle_type handle_;

    constexpr explicit sparse_handle(handle_type handle) : handle_{handle}   {}

public:
    constexpr handle_type handle() const { return handle_; }

    template <class Pool>
    void* lock(Pool& pool) const
    {
        return pool.lock(handle_);
    }

    template <class Pool>
    void unlock(Pool& pool) const
    {
        return pool.unlock(handle_);
    }

    template <class Pool>
    void dealloc(Pool& pool) const
    {
        pool.dealloc(handle_);
    }

    template <class Pool>
    constexpr lock_handle<Pool> lock_handle(Pool& pool) const
    {
        return lock_handle<Pool>(handle_, &pool);
    }

    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool>
    void ref_down(Pool& pool) const
    {
        pool.ops().ref_down(handle_);
    }

    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool>
    void ref_up(Pool& pool) const
    {
        pool.ops().ref_up(handle_);
    }
};

// AKA "sparse handle"
// Not owning
// Not lock-guarded
// Primarily a type-safety mechanism for T
// DEBT: Perhaps actually call this 'sparse_handle' for real
template <class T, class Handle>
class sparse_handle : public sparse_handle<void, Handle>
{
    using base_type = sparse_handle<void, Handle>;

    // Not ready yet
    template <class R, class F>
    R guard_op_ll(estd::false_type, F&& f)
    {
        return f();
    }

    // Not ready yet
    template <class, class F>
    void guard_op_ll(estd::false_type, F&& f)
    {
        f();
    }

protected:
    template <class F>
    estd::invoke_result_t<F> guard_op(F&& f)
    {
        using result_type = estd::invoke_result_t<F>;
        return guard_op_ll<result_type>(estd::is_void<result_type>{}, std::forward<F>(f));
    }

public:
    using handle_type = Handle;

    constexpr explicit sparse_handle(handle_type handle) : base_type{handle}   {}

    template <ESTD_CPP_CONCEPT(mem::concepts::Pool) Pool>
    T* lock(Pool& pool) const
    {
        return static_cast<T*>(base_type::lock(pool));
    }
};

template <class T, class Handle>
constexpr bool operator==(
    const sparse_handle<T, Handle>& lhs,
    const sparse_handle<T, Handle>& rhs)
{
    return lhs.handle() == rhs.handle();
}


#if FEATURE_STD_TYPE_TRAITS
static_assert(std::is_trivially_move_constructible<sparse_handle<int, uint8_t>>::value);
#endif

}}

}}
