#pragma once

#if __cpp_lib_concepts
#include <concepts>

#include "pool/concepts.h"

namespace embr::mem::detail::inline v1::concepts {

template <class T>
concept Handles = requires(T t)
{
    //typename T::size_type;
    t.alloc(int{});
    t.dealloc(int{});
};

}

#endif
