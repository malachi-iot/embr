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
    auto data = (proxy*)data_;          // NOLINT

    new (data) proxy(rt::utility);
    // DEBT: consider a proxy::emplace and/or an in_place_t emplace constructor
    new (data->storage()) T(std::forward<Args>(args)...);
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
            std::memcpy(from->data(), data(), sz);
            mode_ = block::Trivial;
            break;

        case block::RttoProxy:
        {
            auto p = (rtto_proxy*) proxy();
            // FIX: move constructor not yet present for rtto_proxy, but needed
            // as it appears that's the best way to copy underlying u_ (no other means
            // present yet).  Fools us into working because u_ itself gets copied, but
            // move_to isn't called yet
            new (p) rtto_proxy(std::move(*from->proxy()));
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
