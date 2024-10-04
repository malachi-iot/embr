#pragma once

#include <estd/internal/impl/functional/fnptr2.h>

namespace embr { namespace detail { inline namespace v1 {

template <class F, class Objlist>
class funclist
{
    using objlist_type = Objlist;

    Objlist* list_;

    using fnptr = estd::detail::impl::function_fnptr2<F>;
    using pointer = typename objlist_type::pointer;

public:
    template <class F2>
    void operator+=(F2&& f)
    {
        using model = typename fnptr::template model<F2>;

        pointer p = list_->alloc(nullptr, sizeof(model));
    }
};

}}}