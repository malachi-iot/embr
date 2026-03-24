#pragma once

#include "../fwd.h"
#include "../mixins.h"

#include "fwd.h"

namespace embr::mem { inline namespace v1 {

// Works to conform vector_control to https://en.cppreference.com/w/cpp/named_req/Container.html
template <class T, class Pool, Pool* pool>
class pinned<mem::vector<T, Pool, pool>> :
    public lock_guard<mem::detail::vector_control<T>, Pool, pool>,
    public embr::mem::mixins::container<pinned<mem::vector<T, Pool, pool>>, T>
{
    using vector_type = mem::detail::vector_control<T>;
    using base_type = lock_guard<vector_type, Pool, pool>;

public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    using size_type = typename vector_type::size_type;

    pointer data() { return base_type::data()->data(); }
    constexpr const_pointer data() const { return base_type::data()->data(); }

    template <class ...Args>
    constexpr explicit pinned(Args&&...args) : base_type(std::forward<Args>(args)...) {}

    constexpr size_type size() const { return base_type::data()->size_; }

    // DEBT: Put this up in mixin::container, since size_ doesn't need changing
    void swap(pinned& with)
    {
        pointer begin = data();
        const_pointer end = begin + size();
        pointer dbegin = with.data();

        if(size() != with.size())   return; // DEBT: I think we need to do something else here

        for(; begin < end; ++begin, ++dbegin)
            estd::swap(*begin, *dbegin);
    }
};

}}
