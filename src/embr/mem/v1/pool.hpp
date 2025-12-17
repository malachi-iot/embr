#pragma once

#include "pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
template <class Traits2>
typename Traits2::size_type pool<Traits>::alloc(v1::handles<Traits2>& handles, unsigned logical_sz, unsigned block_sz)
{
    return handles.alloc(
        [&](int h, const auto& page)
        {
            v1::block* b = block(page);
            // Check that size is correct
            return true;
        },
        [&](auto& v)
        {
        });
}


}}

}}
