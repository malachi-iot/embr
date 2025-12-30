#pragma once

#include "lock-handle.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

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
