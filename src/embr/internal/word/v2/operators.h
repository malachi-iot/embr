#pragma once

#include <estd/type_traits.h>

#include "fwd.h"

namespace embr { namespace v2 {

// Implicit mode confuses == and we get a billion candidates
template <size_t bits, word_options o>
inline constexpr auto operator ==(
    const word<bits, o>& l,
    const typename word<bits, o>::type& r) ->
    estd::enable_if_t<!(o & word_options::implicit) && !(o & word_options::safe_align), bool>
{
    return l.value() == r;
}


template <size_t bits, word_options o>
inline constexpr auto operator ==(
    const word<bits, o> l,
    const typename word<bits, o>::type& r) ->
    estd::enable_if_t<!(o & word_options::implicit) && o & word_options::safe_align, bool>
{
    return l.value() == r;
}


template <size_t bits1, size_t bits2, word_options o1, word_options o2>
constexpr bool operator ==(const word<bits1, o1>& l, const word<bits2, o2>& r)
{
    return l.equals(r);
}

}}


namespace embr {

// DEBT: Clumsy stuff here - hopefully we can refactor this into embr::detail::v2::word


template <size_t bits, v2::word_options o>
// Compiler gets annoyed when implicit is active here since it identifies two viable == paths
constexpr estd::enable_if_t<!(o & v2::word_options::implicit), bool>
operator==(const internal::word_v2_base<bits, o>& lhs, const typename internal::word_v2_base<bits, o>::type& rhs)
{
    return lhs.equals(rhs);
}

}
