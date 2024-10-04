#pragma once

#include <estd/internal/platform.h>

#include "objstack.h"

namespace embr { namespace detail { inline namespace v1 {

template <int alignment>
struct objlist_element
{
    // In bits
    static constexpr int alignment_ = alignment;

    template <class Int>
    static constexpr Int align(Int v)
    {
        return (v + ((1 << alignment_) - 1) >> alignment_) << alignment_;
    }

    ///< true unaligned size
    unsigned size_ : 16;

    int next_ : 14;    ///< aligned pointer offset

    // Perhaps we can deduce this based on what list it is in?  Don't know.
    // It is convenient to have it here
    bool allocated_ : 1;

    constexpr objlist_element(unsigned size, int next) :
        size_{size},
        next_{next},
        allocated_{false}
    {

    }

    constexpr int next_diff() const
    {
        return next_ << alignment_;
    }

    objlist_element* next() const
    {
        if(next_ == 0)  return nullptr;

        auto base = (char*) this;
        const int delta = next_diff();

        return reinterpret_cast<objlist_element*>(base + delta);
    }

    void next(objlist_element* v)
    {
        auto base = (char*) this;
        auto incoming = (char*) v;
        int delta = (incoming - base) >> alignment_;

        next_ = delta;
    }

    char data_[];
};

// DEBT: Do concept & concept wrapper here
template <class Objstack>
class objlist
{
    Objstack stack_;
    char* base_;

    using objstack_type = Objstack;

public:
    using value_type = objlist_element<2>;
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

            pos += sizeof(value_type) + p->size_;
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
    constexpr objlist(Args&&...args) :
        stack_(std::forward<Args>(args)...),
        base_(stack_.current())
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
        const unsigned offset = sizeof(value_type) + deallocating->size_;

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
