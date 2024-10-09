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
    T::alignment_;
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

enum objlist_element_options
{
    OBJLIST_ELEMENT_NONE = 0,
    OBJLIST_ELEMENT_ALIGN_SIZE = 0x01,
    OBJLIST_ELEMENT_ALWAYS_EXTRA = 0x02
};

#ifndef OBJLIST_ELEMENT_DEFAULT
#define OBJLIST_ELEMENT_DEFAULT embr::detail::v1::OBJLIST_ELEMENT_NONE
#endif

template <int alignment, objlist_element_options options = OBJLIST_ELEMENT_DEFAULT, class T = char>
struct objlist_element;

//template <ESTD_CPP_CONCEPT(concepts::ObjlistElement)>
template <class Element>
class objlist_base;

template <ESTD_CPP_CONCEPT(concepts::Objstack),
    objlist_element_options = OBJLIST_ELEMENT_DEFAULT>
class objlist;

}}}
