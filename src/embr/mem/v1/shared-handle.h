#pragma once

#include "fwd.h"
#include "lock-guard.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Pool, Pool* pool>
class shared_handle : public lock_handle<Pool, pool>
{
    using base_type = lock_handle<Pool, pool>;
    using base_type::value;
    using base_type::is_global;
    using handle_type = typename Pool::handle_type;
    using base_type::ops;

protected:
    using base_type::handle_;

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    constexpr explicit shared_handle(handle_type handle, Pool* p) :
        base_type(handle, p)
    {
        ops().ref_up(handle_);
    }

    constexpr explicit shared_handle(handle_type handle) :
        base_type(handle)
    {
        ops().ref_up(handle_);
    }

    static constexpr bool global = false;

    lock_guard<Pool, pool> operator()() { return { *this }; }

    ~shared_handle()
    {
        if(*this)   ops().ref_down(handle_);
    }
};

}}

inline namespace v1 {

template <class T, class Pool, Pool* pool>
class weak_handle : public detail::v1::lock_handle<Pool, pool>
{
    using base_type = detail::v1::lock_handle<Pool, pool>;
    using shared_type = shared_handle<T, Pool, pool>;

public:
    weak_handle(const shared_type& r) : base_type(r) {}

    shared_type lock()
    {
        return shared_type(base_type::handle_, pool);
    }
};

template <class T, class Pool, Pool* pool>
class shared_handle :
    public detail::shared_handle<Pool, pool>
{
    using base_type = detail::shared_handle<Pool, pool>;

public:
    using weak_type = weak_handle<T, Pool, pool>;

    ESTD_CPP_STD_VALUE_TYPE(T)

    constexpr explicit shared_handle(int handle, Pool* p = nullptr) :
        base_type(handle, p) {}

    lock_guard<value_type, Pool, pool> operator()() { return { *this }; }

    pointer lock() const { return static_cast<pointer>(base_type::lock()); }

    using guard_type = typename base_type::template guard_type<value_type>;
};




// DEBT: Need to filter this more, otherwise ADL is gonna lose its mind
template <class T, class Pool, class ...Args>
constexpr shared_handle<T, Pool> make_shared(Pool& pool, Args&&...args)
{
    return shared_handle<T, Pool>{ pool.template construct<T>(std::forward<Args>(args)...), &pool };
}

}}}
