#pragma once

#include "../block.h"

#include "fwd.h"
#include "ops.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class T>
constexpr block_mode_enum::modes deduce_block_mode()
{
    using is_trivial = estd::is_trivially_constructible<T>;
    using is_rtto_base = estd::is_base_of<estd::internal::rtto_base::base, T>;
    using is_virt_base = estd::is_base_of<estd::internal::rtto_base::virtual_base, T>;
    using modes = block_mode_enum::modes;
    return
        is_trivial::value ? modes::Trivial :
            is_rtto_base::value ? modes::RttoBase :
                is_virt_base::value ? modes::RttoVirtual : modes::RttoProxy;

}


template <class T, class Traits, class ...Args>
typename pool_ops<Traits>::handle_type construct(pool_ops<Traits>& ops, Args&&...args)
{
    constexpr block_modes mode = item_traits<T>::block_mode;

    return ops.template construct<mode, T>(std::forward<Args>(args)...).handle;
}


template <class Traits>
template <block_modes mode, class T, class ...Args>
pool_codes pool_ops<Traits>::construct_ll(bundle* bn, pos_type phys_sz, Args&&...args)
{
    constexpr bool rtto_proxied = mode == block::RttoProxy;

    pool_codes code = alloc(bn, phys_sz, mode);

    if(code != POOL_OK) return code;

    // DEBT: May need this to be if constexpr (or equivalent) - keep an eye on this
    if(rtto_proxied)
        bn->block->template emplace_rtto_proxied<T>(std::forward<Args>(args)...);
    else
        bn->block->template emplace<T>(std::forward<Args>(args)...);

    return code;
}

template <class Traits>
template <block_modes mode, class T, class ...Args>
auto pool_ops<Traits>::construct(Args&&...args) -> bundle
{
    // DEBT: Effective but error prone accounting for various block sizing.  Probably
    // ought to move this plumbing into 'emplace'
    constexpr bytes block_sz = block::header_size(mode);
    bundle bn;

    construct_ll<mode, T>(&bn, do_alias(sizeof(T) + block_sz.count()), std::forward<Args>(args)...);

    return bn;
}



}}

}}
