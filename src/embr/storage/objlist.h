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

    unsigned size_ : 16;
    int next_ : 14;    // aligned pointer offset

    constexpr objlist_element(unsigned size, int next) :
        size_{size},
        next_{next}
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

        return reinterpret_cast<objlist_element*>(base + next_diff());
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

    // Not ready yet
    template <class F>
    void walk(F&& f)
    {
        char* pos = base_;

        while(pos < stack_.current())
        {
            auto p = (pointer) pos;

            f(p);

            //pos += p->next_diff();
        }
    }

public:
    //ESTD_CPP_FORWARDING_CTOR_MEMBER(objlist, stack_)
    template <class ...Args>
    constexpr objlist(Args&&...args) :
        stack_(std::forward<Args>(args)...),
        base_(stack_.current())
    {}

    pointer alloc(size_type sz)
    {
        auto p = (pointer)stack_.alloc(sizeof(value_type) + sz);

        if(p == nullptr)    return nullptr;

        new (p) value_type(sz, 0);

        return p;
    }
};

}}}

namespace embr { namespace layer1 {

template <unsigned N>
using objlist = detail::v1::objlist<detail::v1::layer1::objstack<N>>;

}}
