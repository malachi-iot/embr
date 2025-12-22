#pragma once

#include <estd/internal/units/base.h>
#include <estd/ratio.h>

#include "fwd.h"

// 17DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// DEBT: Rename this guy to reflect he's byte_count
using page_unit_type = estd::internal::units::unit_base<unsigned, estd::ratio<1>, page_tag>;

// TODO: Do unit_traits for human-readable descriptions

#if PAGE_ALIAS == 0
template <class Rep, class Ratio>
struct page
{
    using rep = Rep;
    using unit_type = estd::internal::units::unit_base<Rep, Ratio, page_tag>;

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
