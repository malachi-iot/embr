#pragma once

// DEBT: See if estd has memcpy aliased, pretty sure it does
#include <cstring>

#include <estd/new.h>

#include "block.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {


template <class T, class ...Args>
void block_8::emplace_rtto_proxied(Args&&...args)
{
    using rt = rtto<T>;
    using proxy = typename rt::template proxy<>;

    new (data_) proxy(estd::in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <class T, class ...Args>
void block_8::emplace(Args&&...args)
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

inline void block_8::move_from(this_type* from, unsigned sz)
{
    switch(from->mode_)
    {
        case Trivial:
            std::memmove(data_, from->data_, sz);
            mode_ = Trivial;
            break;

        case RttoProxy:
        {
            new (data_) rtto_proxy(std::move(*from->proxy()), sz);
            mode_ = RttoProxy;
            break;
        }

        case RttoBase:
            from->rtto_base()->move_to(rtto_base());
            mode_ = RttoBase;
            break;

        case Immobile:
            // CANNOT
            return;
    }
}


inline void block_8::destroy()
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
