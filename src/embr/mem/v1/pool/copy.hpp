#pragma once

#include "../pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
auto pool_ops<Traits>::copy(bundle copy_from) -> bundle
{
    pos_type sz = phys_size(copy_from);
    bytes lsz = logical_size(copy_from.mode(), sz);

    bundle copied = alloc(sz, copy_from.mode());

    copied.block->copy_from(copy_from.block, lsz.count());

    return copied;
}

}}

}}
