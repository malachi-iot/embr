#pragma once

#include <estd/internal/platform.h>

#include "fwd.h"
#include "objstack.h"
#include "objlist_element.hpp"

namespace embr { namespace detail { inline namespace v1 {

// DEBT: Creates a concept-dependency loop somehow
//template <ESTD_CPP_CONCEPT(concepts::ObjlistElement) Element>
template <class Element>
class objlist_base
{
protected:
    using value_type = Element;
    using pointer = value_type*;

    // This looks like it can comfortably live in objlist_element itself ... although
    // hiding this function from others is nice

    static void dealloc(pointer prev, pointer p)
    {
        prev->next_ = p->next_;
        p->allocated_ = false;
    }
};

template <ESTD_CPP_CONCEPT(concepts::v1::Objstack) Objstack, objlist_element_options oeo>
class objlist : public objlist_base<objlist_element<Objstack::alignment_, oeo>> // NOLINT(*-pro-type-member-init)
{
    Objstack stack_;
    char* base_;    // DEBT: Somehow linter freaks out about this guy

    using objstack_type = Objstack;
    static constexpr int alignment_ = Objstack::alignment_;
    using base_type = objlist_base<objlist_element<alignment_, oeo>>;

public:
    using value_type = typename base_type::value_type;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = typename objstack_type::size_type;

    template <class T>
    using typed_pointer = objlist_element<alignment_, oeo, T>*;

#if UNIT_TESTING
public:
#else
protected:
#endif

    template <class F>
    void walk(F&& f)
    {
        char* pos = base_;

        while(pos < stack_.current())
        {
            auto p = (pointer) pos;

            f(p);

            pos += p->total_size();
        }
    }

    const objstack_type& stack() const { return stack_; }

public:
    //ESTD_CPP_FORWARDING_CTOR_MEMBER(objlist, stack_)
    template <class ...Args>
    constexpr explicit objlist(Args&&...args) :
        stack_(std::forward<Args>(args)...),
        base_{stack_.current()}
    {}

    template <class T = char>
    typed_pointer<T> alloc(pointer prev, size_type sz)
    {
        auto p = (typed_pointer<T>)stack_.alloc(sizeof(value_type) + sz);

        if(p == nullptr)    return nullptr;

        new (p) value_type(sz, 0, true);

        if(prev)    prev->next(p);

        return p;
    }

    template <class T, class ...Args>
    pointer emplace(pointer prev, Args&&...args)
    {
        // DEBT: Knowing here that we need +'objlist_element_extra' during an emplace is a little sloppy
        pointer p = alloc(prev, sizeof(T) + sizeof(objlist_element_extra));

        if(p == nullptr)    return nullptr;

        p->template emplace<T>(std::forward<Args>(args)...);

        return p;
    }

    void dealloc_next(pointer p)
    {
        pointer deallocating = p->next();

        base_type::dealloc(p, deallocating);

        char* end = stack_.current();
        auto compare = (char*) deallocating;
        const unsigned offset = deallocating->total_size();

        if(compare + offset == end)
        {
            // we got lucky and we're at end of objstack
            stack_.dealloc(offset);
        }
    }
};

}}}

namespace embr {

namespace layer1 { inline namespace v1 {

template <unsigned N,
    unsigned alignment = EMBR_OBJSTACK_DEFAULT_ALIGNMENT,
    detail::objlist_element_options oeo = OBJLIST_ELEMENT_DEFAULT
    >
using objlist = detail::v1::objlist<layer1::v1::objstack<N, alignment>, oeo>;

}}

namespace layer2 { inline namespace v1 {

template <unsigned N,
    unsigned alignment = EMBR_OBJSTACK_DEFAULT_ALIGNMENT,
    detail::objlist_element_options oeo = OBJLIST_ELEMENT_DEFAULT
    >
using objlist = detail::v1::objlist<layer2::v1::objstack<N, alignment>, oeo>;

}}

namespace layer3 { inline namespace v1 {

// UNTESTED
using objlist = detail::v1::objlist<layer3::v1::objstack>;

}}

}
