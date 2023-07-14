#pragma once

#include "bits.h"
#include "word.hpp"

#include "be-get.hpp"
#include "be-set.hpp"
#include "le-get.hpp"
#include "le-set.hpp"

namespace embr { namespace bits {

namespace experimental {

template <unsigned bitpos, unsigned length, endianness e,
    length_direction d, resume_direction rd,
    typename TInt, class TIt>
inline void set(TIt raw, TInt v)
{
    typedef detail::setter<bitpos, length, e, d, rd> s;

    s::set(raw + s::adjuster(), v);
}

}

template <endianness e, class TBase, class TBase2>
ESTD_CPP_CONSTEXPR_RET bool operator <(
    const embr::bits::internal::provider<e, TBase>& lhs,
    const embr::bits::internal::provider<e, TBase2>& rhs)
{
    return embr::bits::internal::compare<e, false, false>::eval(lhs, rhs);
}

template <endianness e, class TBase, class TBase2>
ESTD_CPP_CONSTEXPR_RET bool operator >(
    const embr::bits::internal::provider<e, TBase>& lhs,
    const embr::bits::internal::provider<e, TBase2>& rhs)
{
    return embr::bits::internal::compare<e, true, false>::eval(lhs, rhs);
}

template <endianness e, class TBase, class TBase2>
inline bool operator ==(
    const embr::bits::internal::provider<e, TBase>& lhs,
    const embr::bits::internal::provider<e, TBase2>& rhs)
{
    if(lhs.size() != rhs.size()) return false;

    // DEBT
    const uint8_t* lhs_begin = lhs.begin();
    const uint8_t* lhs_end = lhs.end();

    // Doesn't matter if big endian or little endian if comparing the whole range
    return estd::equal(lhs_begin, lhs_end, rhs.begin());

    unsigned sz = lhs.size();

    if(sz != rhs.size()) return false;

    auto lhs_i = lhs.begin();
    auto rhs_i = rhs.begin();

    // TODO: Use equal_to
    while(sz--) if(*lhs_i++ != *rhs_i++) return false;

    return true;
}

template <endianness e, typename TInt, length_direction ld, resume_direction rd, typename TByte>
inline TInt get(descriptor d, const TByte* raw)
{
    return getter<e, ld, rd>::template get<TInt>(d, raw);
}

template <endianness e, typename TInt, typename TByte>
inline TInt get(const TByte* raw)
{
    typedef experimental::byte_boundary_getter<e, TInt> g;

    TInt v;

    g::get(raw + g::adjuster(), v);

    return v;
}


template <endianness e, typename TInt, length_direction ld, resume_direction rd, typename TByte>
inline void set(descriptor d, TByte* raw, TInt v)
{
    typedef setter<e, ld, rd> setter;

    setter::set(d, raw, v);
}


template <endianness e, typename TInt, typename TByte>
inline void set(TByte* raw, TInt v)
{
    typedef experimental::byte_boundary_setter<e, TInt> s;

    s::set(raw + s::adjuster(), v);
}


}}