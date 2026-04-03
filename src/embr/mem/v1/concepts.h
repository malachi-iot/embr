#pragma once

#if __cpp_lib_concepts
#include <concepts>

#include "pool/concepts.h"

namespace embr::mem {

namespace detail::inline v1::concepts {

template <class T>
concept Handles = requires(T t)
{
    //typename T::size_type;
    t.alloc(int{});
    t.dealloc(int{});
};

}   // detail::v1::concepts

inline namespace v1 {

namespace concepts {

template <class T>
concept Pool =
    detail::concepts::HandlesTraits<typename T::handles_traits> &&
    requires(T t)
{
    typename T::handle_type;

    t.lock(typename T::handle_type{});
    t.unlock(typename T::handle_type{});

    t.alloc(int{});
    t.dealloc(typename T::handle_type{});
    //t.construct();
};

}

}

}

#endif
