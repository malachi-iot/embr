#pragma once

#include <estd/internal/impl/functional/fnptr2.h>

#include "fwd.h"

namespace embr { namespace detail { inline namespace v1 {

namespace impl {

template <class F, ESTD_CPP_CONCEPT(concepts::v1::ObjlistElement) Element>
struct funclist
{
    using value_type = Element;
    using pointer = Element*;

    pointer head_ = nullptr;
    using fnptr = estd::detail::impl::function_fnptr2<F>;

    constexpr bool empty() const { return head_ == nullptr; }

    template <class Objlist, class F2>
    void add(Objlist& list, F2&& f)
    {
        using model = typename fnptr::template model<F2>;

        pointer p = list.alloc(nullptr, sizeof(model));

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

    template <class Objlist, class ...Args>
    void fire(Objlist& list, Args&&...args)
    {
        using model = typename fnptr::model_base;

        head_->walk([&](pointer p, pointer)
        {
            auto m = (model*)p->data();

            m->operator()(std::forward<Args>(args)...);
        });
    }
};

}

template <class F, ESTD_CPP_CONCEPT(concepts::v1::Objlist) Objlist>
class funclist
{
    using objlist_type = Objlist;
    using value_type = typename objlist_type::value_type;
    using pointer = typename objlist_type::pointer;

    Objlist* list_;
    impl::funclist<F, value_type> impl_;

public:
    constexpr explicit funclist(objlist_type* objlist) :
        list_{objlist}
    {

    }

    template <class F2>
    void operator+=(F2&& f)
    {
        impl_.add(*list_, std::forward<F2>(f));
    }

    template <class ...Args>
    void fire(Args&&...args)
    {
        impl_.fire(*list_, std::forward<Args>(args)...);
    }
};

}}}