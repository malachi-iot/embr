#pragma once

#if __cpp_concepts
#include <concepts>
#endif

namespace embr { namespace detail { inline namespace v1 {

namespace concepts { inline namespace v1 {

#if __cpp_concepts
template <class T>
concept Objstack = requires(T v)
{
    v.alloc();
};
#endif

}}

template <unsigned alignment>
class objlist_base;

template <class Objstack, unsigned alignment = 2>
class objlist;

}}}
