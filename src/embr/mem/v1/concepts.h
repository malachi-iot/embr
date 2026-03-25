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

}

}

}

#endif
