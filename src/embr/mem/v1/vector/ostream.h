#pragma once

#include <estd/internal/platform.h>

#if FEATURE_STD_OSTREAM
#include <estd/iosfwd.h>
#endif

#include "fwd.h"

namespace embr { namespace mem { inline namespace v1 {

#if FEATURE_STD_OSTREAM && EMBR_VECTOR_ADV_ACCESSOR
// FIX: ADL doesn't seem to pick this up, perhaps because they are inner classes?
template <class Char, class T, class Pool, Pool* pool>
std::basic_ostream<Char>& operator <<(std::basic_ostream<Char>& out,
    const typename detail::vector<T, Pool, pool>::allocator_traits::accessor& acc)
{
    return out << acc.value();
}
#endif

}}} // embr::mem::v1