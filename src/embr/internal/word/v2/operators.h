#include "fwd.h"

namespace embr { namespace v2 {

template <size_t bits, word_options o, typename Int>
constexpr bool operator ==(const word<bits, o>& l, Int r)
{
    return l.value() == r;
}

template <size_t bits1, size_t bits2, word_options o1, word_options o2>
constexpr bool operator ==(const word<bits1, o1>& l, const word<bits2, o2>& r)
{
    return l.equals(r);
}

}}
