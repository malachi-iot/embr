#pragma once

#include <estd/internal/impl/functional/fnptr2.h>

#include "fwd.h"

namespace embr { namespace detail { inline namespace v1 {

namespace internal {

template <class F>
struct objlist_function_factory
{
    using fn_impl = estd::detail::impl::function_fnptr2<F>;
    using model = typename fn_impl::model_base;

    template <ESTD_CPP_CONCEPT(concepts::v1::Objlist) Objlist, class F2>
    static typename Objlist::pointer emplace(typename Objlist::pointer prev, Objlist& list, F2&& f)
    {
        using m = typename fn_impl::template model<F2>;

        return list.template emplace<m>(prev, std::forward<F2>(f));
    }

    template <ESTD_CPP_CONCEPT(concepts::v1::Objlist) Objlist, class F2>
    static typename Objlist::pointer emplace(Objlist& list, F2&& f)
    {
        return emplace(nullptr, list, std::forward<F2>(f));
    }
};

}

namespace impl {

template <class F, ESTD_CPP_CONCEPT(concepts::v1::ObjlistElement) Element>
struct funclist
{
    using value_type = Element;
    using pointer = Element*;

    pointer head_ = nullptr;
    using factory = internal::objlist_function_factory<F>;
    using fn_impl = typename factory::fn_impl;

    constexpr bool empty() const { return head_ == nullptr; }

    template <ESTD_CPP_CONCEPT(concepts::v1::Objlist) Objlist, class F2>
    pointer add(Objlist& list, F2&& f)
    {
        /*
        using model = typename fnptr::template model<F2>;

        pointer p = list.template emplace<model>(nullptr, std::forward<F2>(f)); */
        pointer p = factory::emplace(
            nullptr,
            list,
            std::forward<F2>(f));

        if(p == nullptr) return nullptr;

        if(empty())
        {
            head_ = p;
        }
        else
        {
            head_->get_last()->next(p);
        }

        return p;
    }

    template <class Objlist, class ...Args>
    void fire(Objlist& list, Args&&...args)
    {
        using model = typename fn_impl::model_base;

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