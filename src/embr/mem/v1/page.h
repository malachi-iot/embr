#pragma once

#include <estd/ratio.h>
#include <estd/units.h>

#include "fwd.h"

// 17DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

namespace embr { namespace mem {

namespace detail { inline namespace v1 {


// DEBT: Precision loss checker in estd += is not specific enough to notice traits and page_unit_traits
// are compatible.
template <typename Rep, typename Period>
using page_unit_traits = estd::units::v1::detail::traits<Rep, Period, page_tag>;
/*
struct page_unit_traits : estd::units::detail::traits<Rep, Period, page_tag>
{
    // TODO: Put in init options
};  */

// DEBT: Rename this guy to reflect he's byte_count
using page_unit_type = estd::units::v1::detail::unit<page_unit_traits<unsigned, estd::ratio<1>>>;

// TODO: Do unit_traits for human-readable descriptions

#if PAGE_ALIAS == 0
template <class Rep, class Ratio>
struct page
{
    using rep = Rep;
    using unit_type = estd::units::v1::detail::unit<page_unit_traits<Rep, Ratio>>;

    static constexpr int aliasing = Ratio::num;
    static_assert(aliasing % sizeof(void*) == 0, "Aliasing must fall on pointer size boundary");
    static constexpr rep null = estd::numeric_limits<rep>::max();

    constexpr bool is_null() const { return pos_ == null; }
    void reset() { pos_ = null; }

    constexpr unit_type pos() const { return unit_type{ pos_ }; }
    void pos(unit_type v) { pos_ = v.count(); }

private:
    rep pos_;
};
#endif


}}

}}
