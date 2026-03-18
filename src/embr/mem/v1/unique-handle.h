#pragma once

#include "fwd.h"
#include "lock-guard.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Pool, Pool* pool>
class unique_handle : public lock_handle<Pool, pool>
{
    using base_type = lock_handle<Pool, pool>;

protected:
    using base_type::value;
    using base_type::is_global;
    using base_type::handle_;
    using typename base_type::handle_type;
    using base_type::ops;

public:
    constexpr explicit unique_handle(handle_type handle, Pool* p = nullptr) :
        base_type(handle, p)
    {
    }

    unique_handle(const unique_handle&) = delete;

    ~unique_handle()
    {
        if(*this)   ops().dealloc(handle_);
    }
};

}}

inline namespace v1 {

template <class T, class Pool, Pool* pool>
class unique_handle : public detail::unique_handle<Pool, pool>
    // DEBT: Why don't we use crtp/mixin here?
    //detail::v1::lock_handle_crtp<unique_handle<T, Pool, pool>>
{
    using base_type = detail::unique_handle<Pool, pool>;

public:
    using typename base_type::handle_type;
    using pool_type = Pool;

    ESTD_CPP_STD_VALUE_TYPE(T)

    constexpr explicit unique_handle(handle_type handle, Pool* p = nullptr) :
        base_type(handle, p) {}


    pointer lock() const { return static_cast<pointer>(base_type::lock()); }
    const_pointer clock() const { return static_cast<const_pointer>(base_type::lock()); }

    typename base_type::template guard_type<T> guard() const
    {
        return { *this };
    }

protected:
    // Low-level call which presumes we're already locked
    pointer data()
    {
        return static_cast<pointer>(base_type::data());
    }

    const_pointer data() const
    {
        // DEBT: This feels like it ought to live in a mixin
        typename base_type::const_bundle bn(base_type::get_bundle());

        // DEBT: Put in a strict-mode flag to avoid this check for optimizations
        assert(bn.block->lock_count() > 0);

        return static_cast<const_pointer>(bn.data());
    }
};

}

}}
