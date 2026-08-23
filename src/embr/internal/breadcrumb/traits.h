#pragma once

#include "fwd.h"

#include <estd/string_view.h>

namespace embr { namespace internal {

template <class T>
struct breadcrumb_traits_base
{
    using value_type = T;
    using reference = const T&;

    using int_type = estd::remove_cvref_t<decltype(T::id)>;

    //static constexpr bool is_null(const T& v) { return v.name.empty(); }
    static constexpr bool is_null(reference v) { return v.id == T::null_id; }
    static constexpr bool is_child(reference parent, reference child)
    {
        return parent.id == child.parent;
    }
    static constexpr bool is_sibling(reference lhs, reference rhs)
    {
        return lhs.parent == rhs.parent;
    }
    static constexpr bool equals(reference lhs, reference rhs)
    {
        return lhs.id == rhs.id;
    }
};

template <class T>
struct breadcrumb_traits : breadcrumb_traits_base<T>
{
    static constexpr const estd::string_view& name(const T& v) { return v.name; }
};

}}