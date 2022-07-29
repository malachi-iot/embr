#pragma once

#include "bits.h"
#include "word.hpp"

#include "be-get.hpp"
#include "be-set.hpp"
#include "le-get.hpp"
#include "le-set.hpp"

namespace embr { namespace bits {

template <endianness e, class TBase>
inline bool operator <(
    const embr::bits::internal::provider<e, TBase>& lhs,
    const embr::bits::internal::provider<e, TBase>& rhs)
{
    return embr::bits::internal::compare<e, false, false>::eval(lhs, rhs);
}

template <endianness e, class TBase>
inline bool operator >(
    const embr::bits::internal::provider<e, TBase>& lhs,
    const embr::bits::internal::provider<e, TBase>& rhs)
{
    return embr::bits::internal::compare<e, true, false>::eval(lhs, rhs);
}

template <endianness e, class TBase>
inline bool operator ==(
    const embr::bits::internal::provider<e, TBase>& lhs,
    const embr::bits::internal::provider<e, TBase>& rhs)
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
    return internal::getter<TInt, e, ld, rd>::get_adjusted(d, raw);
}

template <endianness e, typename TInt, typename TByte>
inline TInt get(const TByte* raw)
{
    typedef experimental::byte_boundary_getter<e, no_direction> g;

    TInt v;

    // DEBT: Fix byte_boundary_getter itself to not take direction and instead take TInt.
    // this way 'd' won't be needed at all here
    descriptor d{0, sizeof(TInt) * byte_size()};

    g::get(d, raw + g::adjuster(d), v);

    return v;
}


template <endianness e, typename TInt, length_direction ld, resume_direction rd, typename TByte>
inline void set(descriptor d, TByte* raw, TInt v)
{
    typedef internal::setter<TInt, e, ld, rd> setter;

    setter::set(d, raw, v);
}


template <endianness e, typename TInt, typename TByte>
inline void set(TByte* raw, TInt v)
{
    typedef internal::setter<TInt, e, no_direction> setter;

    setter::set(raw, v);
}


}}