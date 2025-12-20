#pragma once

#include "block.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class T, class ...Args>
block::block(estd::in_place_index_t<Immobile>, estd::in_place_type_t<T>, Args&&...args) :
    mode_{Immobile}
{
}

template <class T, class ...Args>
block::block(estd::in_place_index_t<Trivial>, estd::in_place_type_t<T>, Args&&...args) :
    mode_{Trivial}
{

}

template <class T, class ...Args>
block::block(estd::in_place_index_t<Rtto>, estd::in_place_type_t<T>, Args&&...args) :
    mode_{Rtto}
{
    using rt = rtto<T>;
    using proxy = typename rt::template proxy<>;
    auto data = (proxy*)data_;

    new (data) proxy(rt::utility);
    // DEBT: consider a proxy::emplace and/or an in_place_t emplace constructor
    new (data->storage()) T(std::forward<Args>(args)...);
}


template <class T, class ...Args>
block::block(estd::in_place_index_t<modes::RttoBase>, estd::in_place_type_t<T>, Args&&...args) :
    mode_{RttoBase}
{

}


}}

}}
