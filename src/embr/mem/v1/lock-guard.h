#pragma once

#include "lock-handle.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// DEBT: Perhaps call this lock_data?
template <class Pool, Pool* pool, class T = void>
class lock_guard_base : public lock_handle<Pool, pool>
{
    using base_type = lock_handle<Pool, pool>;

protected:
    T* data_;

    // The exact lock acquisition of 'data' can be nuanced, so caller determines that
    // and is responsible for actual lock operation
    template <class ...Args>
    explicit constexpr lock_guard_base(T* data, Args&&...args) :
        base_type(std::forward<Args>(args)...),
        data_(data)
    {}

    constexpr lock_guard_base(const lock_guard_base& copy_from) :
        data_{copy_from.data_}
    {}

    lock_guard_base(lock_guard_base&& move_from) :
        data_{move_from.data_}
    {
        move_from.data_ = nullptr;
    }

    ~lock_guard_base()
    {
        if(data_)   base_type::unlock();
    }

public:
    T* data() const { return data_; }
};


template <class Pool, Pool* pool>
class lock_guard : public lock_guard_base<Pool, pool>
{
    using hnd = lock_handle<Pool, pool>;
    using base_type = lock_guard_base<Pool, pool>;

public:
    // DEBT: We can do better than an intermediate hnd here
    constexpr lock_guard(typename base_type::handle_type h, Pool* p) :
        base_type{hnd{h, p}.lock(), h, p}
    {
    }

    constexpr explicit lock_guard(hnd h) :
        base_type{h.lock(), h}
    {
    }

    constexpr lock_guard(const lock_guard& copy_from) = default;
    constexpr lock_guard(lock_guard&& move_from) = default;
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

protected:
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
