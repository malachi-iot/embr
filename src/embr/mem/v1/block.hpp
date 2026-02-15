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


inline void block_8::copy_from(this_type* from, unsigned sz)
{
    switch(from->mode_)
    {
        case Trivial:
            std::memcpy(data_, from->data_, sz);
            mode_ = Trivial;
            break;

        case RttoProxy:
        {
            new (data_) rtto_proxy(*from->proxy());
            mode_ = RttoProxy;
            break;
        }

        case RttoBase:
            from->rtto_base()->copy_to(rtto_base());
            mode_ = RttoBase;
            break;
    }
}

inline void block_8::move_from(this_type* from, unsigned sz)
{
    switch(from->mode_)
    {
        case Trivial:
            // Moves can overlap, so use memmove for its resiliency to that
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
    }
}


inline void block_8::destroy()
{
    assert(lock_count_ == 0);

    switch(mode_)
    {
        case Trivial:
            break;

        case RttoProxy:
            proxy()->destroy();
            break;

        case RttoBase:
            rtto_base()->destroy();
            break;
    }
}


inline auto block_8::metadata() const -> metadata_type
{
    metadata_type md = nullptr;

#if FEATURE_ESTD_RTTO_GET_METADATA
    switch(mode_)
    {
        case RttoProxy:
        {
            int rc = proxy()->get_metadata(&md);
            break;
        }

        case RttoBase:
        {
            int rc = rtto_base()->get_metadata(&md);
            break;
        }

        default:
            break;
    }
#endif

    return md;
}


inline const char* block_mode_enum::to_string(modes mode)
{
    switch(mode)
    {
        case Trivial:       return "Trivial";
        case RttoProxy:     return "RttoProxy";
        case RttoBase:      return "RttoBase";
        default:            return "N/A";
    }
}


inline char to_abbrev(block_mode_enum::modes m)
{
    using B = block_mode_enum::modes;

    switch(m)
    {
        case B::Trivial:    return 'T';
        case B::RttoProxy:  return 'P';
        case B::RttoBase:   return 'B';
        default:            return '?';
    }
}



}}

}}
