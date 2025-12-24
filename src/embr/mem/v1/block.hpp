#pragma once

// DEBT: See if estd has memcpy aliased, pretty sure it does
#include <cstring>

#include <estd/new.h>

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
block::block(estd::in_place_index_t<RttoProxy>, estd::in_place_type_t<T>, Args&&...args) :
    mode_{RttoProxy}
{
    emplace_rtto_proxied<T>(std::forward<Args>(args)...);
}


template <class T, class ...Args>
block::block(estd::in_place_index_t<modes::RttoBase>, estd::in_place_type_t<T>, Args&&...args) :
    mode_{RttoBase}
{

}

template <class T, class ...Args>
void block::emplace_rtto_proxied(Args&&...args)
{
    using rt = rtto<T>;
    using proxy = typename rt::template proxy<>;

    new (data_) proxy(estd::in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <class T, class ...Args>
void block::emplace(Args&&...args)
{
    new (data_) T(std::forward<Args>(args)...);
}

// DEBT: Let's make a .cpp for this - mem pool does deserve its own
/*
inline block& block::operator=(block&& move_from)
{
    // NOTE: Can't do this because we need explicit sz mentioned
    return *this;
}
*/

inline void block::move_from(block* from, unsigned sz)
{
    switch(from->mode_)
    {
        case block::Trivial:
            std::memcpy(data_, from->data_, sz);
            mode_ = block::Trivial;
            break;

        case block::RttoProxy:
        {
            new (data_) rtto_proxy(std::move(*from->proxy()), sz);
            mode_ = block::RttoProxy;
            break;
        }

        case block::RttoBase:
            from->rtto_base()->move_to(rtto_base());
            mode_ = block::RttoBase;
            break;

        case block::Immobile:
            // CANNOT
            return;
    }
}


inline void block::destroy()
{
    assert(lock_count_ == 0);

    switch(mode_)
    {
        case Trivial:
            break;

        case Immobile:
        case RttoProxy:
            proxy()->destroy();
            break;

        case RttoBase:
            rtto_base()->destroy();
            break;
    }
}



}}

}}
