#pragma once

#include <estd/type_traits.h>

#include "fwd.h"

namespace embr { namespace v2 {

// Implicit mode confuses == and we get a billion candidates
template <size_t bits, word_options o>
constexpr estd::enable_if_t<!(o & word_options::implicit), bool> operator ==(
    const word<bits, o>& l,
    const typename word<bits, o>::type& r)
{
    return l.value() == r;
}

template <size_t bits1, size_t bits2, word_options o1, word_options o2>
constexpr bool operator ==(const word<bits1, o1>& l, const word<bits2, o2>& r)
{
    return l.equals(r);
}

}}
