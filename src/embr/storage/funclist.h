#pragma once

#include <estd/internal/impl/functional/fnptr2.h>

#include "fwd.h"

namespace embr { namespace detail { inline namespace v1 {

namespace impl {

template <class F, class Element>
struct funclist
{
    using value_type = Element;
    using pointer = Element*;
};

}

template <class F, class Objlist>
class funclist
{
    using objlist_type = Objlist;

    Objlist* list_;

    using fnptr = estd::detail::impl::function_fnptr2<F>;
    using pointer = typename objlist_type::pointer;

    pointer head_ = nullptr;

public:
    constexpr funclist(objlist_type* objlist) :
        list_{objlist}
    {

    }

    constexpr bool empty() const { return head_ == nullptr; }

    template <class F2>
    void operator+=(F2&& f)
    {
        using model = typename fnptr::template model<F2>;

        pointer p = list_->alloc(nullptr, sizeof(model));

        if(p == nullptr) return;

        p->template emplace<model>(std::forward<F2>(f));

        if(empty())
        {
            head_ = p;
        }
        else
        {
            head_->last()->next(p);
        }
    }

    template <class ...Args>
    void fire(Args&&...args)
    {
        using model = typename fnptr::model_base;

        // DEBT: Combining everything into objlist makes knowing which 'walk' to
        // call a little confusing
        list_->walk(head_, [&](pointer p, pointer)
        {
            auto m = (model*)p->data();

            m->operator()(std::forward<Args>(args)...);
        });
    }
};

}}}