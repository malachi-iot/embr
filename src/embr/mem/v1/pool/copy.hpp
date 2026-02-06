#pragma once

#include "../pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
auto pool_ops<Traits>::copy(bundle copy_from) -> bundle
{
    return {};
}

}}

}}
