#pragma once

#include "../block.h"

#include "fwd.h"
#include "ops.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class T>
constexpr block_mode_enum::modes ascertain_block_mode()
{
    using is_trivial = estd::is_trivially_constructible<T>;
    using is_rtto_base = estd::is_base_of<estd::internal::rtto_base::base, T>;
    using modes = block_mode_enum::modes;
    return
        is_trivial::value ? modes::Trivial :
            is_rtto_base::value ? modes::RttoBase : modes::RttoProxy;

}


template <class T, class PoolTraits, class HandlesTraits, class ...Args>
typename HandlesTraits::handle_type construct(pool<PoolTraits>& p, handles<HandlesTraits>& h, Args&&...args)
{
    constexpr block_mode_enum::modes mode = ascertain_block_mode<T>();
    using traits = pool_ops_val_traits<pool<PoolTraits>&, handles<HandlesTraits>&>;

    return pool_ops<traits>{p, h}.template construct<mode, T>(std::forward<Args>(args)...).handle;
}


template <class Traits>
template <block_mode_enum::modes mode, class T, class ...Args>
auto pool_ops<Traits>::construct_ll(pos_type phys_sz, Args&&...args) -> bundle
{
    constexpr bool rtto_proxied = mode == block::RttoProxy;

    bundle bn = alloc(phys_sz, mode);

    if(bn.is_null())    return {};

    // DEBT: May need this to be if constexpr (or equivalent) - keep an eye on this
    if(rtto_proxied)
        bn.block->template emplace_rtto_proxied<T>(std::forward<Args>(args)...);
    else
        bn.block->template emplace<T>(std::forward<Args>(args)...);

    return bn;
}

template <class Traits>
template <block_mode_enum::modes mode, class T, class ...Args>
auto pool_ops<Traits>::construct(Args&&...args) -> bundle
{
    // DEBT: Effective but error prone accounting for various block sizing.  Probably
    // ought to move this plumbing into 'emplace'
    constexpr bytes block_sz = block::header_size(mode);

    return construct_ll<mode, T>(do_alias(sizeof(T) + block_sz.count()), std::forward<Args>(args)...);
}



}}

}}
