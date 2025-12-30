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
    using base_type::handle_;
    using handle_type = typename Pool::handle_type;

public:
    // NOTE: Out of order - expect Pool, handle order in anticipation of nullptr p
    constexpr explicit shared_handle(handle_type handle, Pool* p) :
        base_type(handle, p)
    {
        value()->ops().ref_up(handle_);
    }

    constexpr explicit shared_handle(handle_type handle) :
        base_type(handle)
    {
        value()->ops().ref_up(handle_);
    }

    static constexpr bool global = false;

    lock_guard<Pool, pool> guard() { return { *this }; }

    ~shared_handle()
    {
        if(*this)   value()->ops().ref_down(handle_);
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

    constexpr explicit shared_handle(int handle, Pool* p = nullptr) :
        base_type(handle, p) {}

    lock_guard<value_type, Pool, pool> guard() { return { *this }; }

    pointer lock() const { return static_cast<pointer>(base_type::lock()); }
};




// DEBT: Need to filter this more, otherwise ADL is gonna lose its mind
template <class T, class Pool, class ...Args>
constexpr shared_handle<T, Pool> make_shared(Pool& pool, Args&&...args)
{
    return shared_handle<T, Pool>{ pool.template construct<T>(std::forward<Args>(args)...), &pool };
}

}}}
