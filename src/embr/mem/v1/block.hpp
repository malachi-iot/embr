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
    //using proxy = estd::internal::rtto<T>::
}


template <class T, class ...Args>
block::block(estd::in_place_index_t<modes::RttoBase>, estd::in_place_type_t<T>, Args&&...args) :
    mode_{RttoBase}
{

}


}}

}}
