#pragma once

#if __cpp_concepts
#include <concepts>
#endif

#ifndef EMBR_OBJSTACK_DEFAULT_ALIGNMENT
// In bits (0=1 byte aligned, 1=2, 2=4, etc)
#define EMBR_OBJSTACK_DEFAULT_ALIGNMENT 2
#endif


namespace embr { namespace detail { inline namespace v1 {

typedef void (*objlist_element_move_fn)(void* source, void* dest);

namespace concepts { inline namespace v1 {

#if __cpp_concepts
template <class T>
concept Objstack = requires(T v)
{
    v.alloc(0);
};

template <class T>
concept Objlist = requires(T v)
{
    v.alloc({}, 0);
};

template <class T>
concept ObjlistElement = requires(T v)
{
    v.size();
    v.next();
};
#endif

}}

template <unsigned alignment, bool always_extra>
class objlist_base;

template <ESTD_CPP_CONCEPT(concepts::Objstack) Objstack,
    unsigned alignment = EMBR_OBJSTACK_DEFAULT_ALIGNMENT,
    bool always_extra = false>
class objlist;

}}}
