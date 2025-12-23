#pragma once

#include <estd/internal/value_evaporator.h>

#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// Overlap with estd::experimental::global_provider

template <class T, T* t, bool global = estd::enable_if<t != nullptr>::value>
struct global_provider;

template <class T, T* t>
struct global_provider<T, t, true>
{
    T* value() { return t; }

    constexpr global_provider(T*) {}
};

template <class T, T* t>
struct global_provider<T, t, false>
{
    T* t_;

    T* value() { return t_; }

    constexpr global_provider(T* v) : t_{v}   {}
};


template <class Pool, Pool* pool>
class shared_handle : public global_provider<Pool, pool>
{
    using base_type = global_provider<Pool, pool>;
    using base_type::value;

    int handle_;

public:
    constexpr shared_handle(Pool* p) : base_type{p} {}

    static constexpr bool global = false;

    void lock()
    {
        value()->lock(handle_);
    }

    void unlock()
    {
        value()->unlock(handle_);
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
    shared_handle(int handle, Pool* p = nullptr) : base_type(p) {}
};

}}}
