#pragma once

#include <estd/ratio.h>
#include <estd/units.h>

#include "fwd.h"

// 17DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

namespace embr { namespace mem {

namespace detail { inline namespace v1 {


template <typename Rep, typename Period>
struct page_unit_traits : estd::units::detail::traits<Rep, Period, page_tag>
{
    static constexpr auto options = estd::units::detail::options::value_initialized;

    constexpr static Rep default_value() { return estd::numeric_limits<Rep>::max(); }
};

template <typename Rep, typename Period>
using page_unit_type_base = estd::units::v1::detail::unit<page_unit_traits<Rep, Period>>;

// DEBT: Rename this guy to reflect he's byte_count - already bit me a few times
// DEBT: default_value() is not gonna be accurate because of precision difference
using page_unit_type = page_unit_type_base<unsigned, estd::ratio<1>>;

// TODO: Do unit_traits for human-readable descriptions

#if PAGE_ALIAS == 0
template <class Rep, class Ratio>
struct page
{
    using rep = Rep;
    using unit_traits = page_unit_traits<Rep, Ratio>;
    using unit_type = estd::units::v1::detail::unit<unit_traits>;

    static constexpr int aliasing = Ratio::num;
    static_assert(aliasing % sizeof(void*) == 0, "Aliasing must fall on pointer size boundary");
    static constexpr rep null = unit_traits::default_value();

    constexpr bool is_null() const { return pos_ == null; }
    void reset() { pos_ = null; }

    constexpr unit_type pos() const { return unit_type{ pos_ }; }
    void pos(unit_type v) { pos_ = v.count(); }

private:
    rep pos_{null};

public:
    friend void swap(page& lhs, page& rhs) noexcept
    {
        std::swap(lhs.pos_, rhs.pos_);
    }

};

#endif


}}

}}
