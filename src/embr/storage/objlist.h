#pragma once

#include <estd/internal/platform.h>

#include "objstack.h"
#include "objlist_element.h"

namespace embr { namespace detail { inline namespace v1 {

template <unsigned alignment>
class objlist_base
{
protected:
    using value_type = objlist_element<alignment>;
    using pointer = value_type*;

    void alloc(pointer p, unsigned sz)
    {
        new (p) value_type(sz, 0);

        p->allocated_ = true;
    }

    void dealloc(pointer prev, pointer p)
    {
        prev->next_ = p->next_;
        p->allocated_ = false;
    }
};

// DEBT: Do concept & concept wrapper here
template <class Objstack, unsigned alignment>
class objlist : public objlist_base<alignment> // NOLINT(*-pro-type-member-init)
{
    Objstack stack_;
    char* base_;    // DEBT: Somehow linter freaks out about this guy

    using base_type = objlist_base<alignment>;
    using objstack_type = Objstack;

public:
    using value_type = typename base_type::value_type;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = typename objstack_type::size_type;

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

            // DEBT: Can optimize if p->size() is already in alignment mode
            pos += sizeof(value_type) + align<alignment>(p->size());
        }
    }

    template <class F>
    void walk(pointer current, F&& f)
    {
        while(current != nullptr)
        {
            pointer next = current->next();

            f(current, next);

            current = next;
        }
    }

public:
    //ESTD_CPP_FORWARDING_CTOR_MEMBER(objlist, stack_)
    template <class ...Args>
    constexpr explicit objlist(Args&&...args) :
        stack_(std::forward<Args>(args)...),
        base_{stack_.current()}
    {}

    pointer alloc(pointer prev, size_type sz)
    {
        auto p = (pointer)stack_.alloc(sizeof(value_type) + sz);

        if(p == nullptr)    return nullptr;

        base_type::alloc(p, sz);

        if(prev)    prev->next(p);

        return p;
    }

    void dealloc_next(pointer p)
    {
        pointer deallocating = p->next();

        base_type::dealloc(p, deallocating);

        char* end = stack_.current();
        auto compare = (char*) deallocating;
        const unsigned offset = sizeof(value_type) +
            align<alignment>(deallocating->size());

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

template <unsigned N>
using objlist = detail::v1::objlist<layer1::v1::objstack<N>>;

}}

namespace layer2 { inline namespace v1 {

template <unsigned N>
using objlist = detail::v1::objlist<layer2::v1::objstack<N>>;

}}

namespace layer3 { inline namespace v1 {

// UNTESTED
using objlist = detail::v1::objlist<layer3::v1::objstack>;

}}

}