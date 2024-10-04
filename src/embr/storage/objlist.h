#pragma once

#include <estd/internal/platform.h>

#include "objstack.h"
#include "objlist_element.h"

namespace embr { namespace detail { inline namespace v1 {

// DEBT: Do concept & concept wrapper here
template <class Objstack, unsigned alignment = 2>
class objlist
{
    Objstack stack_;
    char* base_{};

    using objstack_type = Objstack;

public:
    using value_type = objlist_element<alignment>;
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

        new (p) value_type(sz, 0);

        if(prev)    prev->next(p);

        p->allocated_ = true;

        return p;
    }

    void dealloc_next(pointer p)
    {
        pointer deallocating = p->next();

        p->next_ = deallocating->next_;

        char* end = stack_.current();
        auto compare = (char*) deallocating;
        const unsigned offset = sizeof(value_type) + deallocating->size();

        deallocating->allocated_ = false;

        if(compare + offset == end)
        {
            // we got lucky and we're at end of objstack
            stack_.dealloc(offset);
        }
    }
};

}}}

namespace embr { namespace layer1 {

template <unsigned N>
using objlist = detail::v1::objlist<detail::v1::layer1::objstack<N>>;

}}
