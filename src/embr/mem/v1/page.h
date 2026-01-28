#pragma once

#include "fwd.h"
#include "unit.h"

// 17DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

namespace embr { namespace mem {

namespace detail { inline namespace v1 {


#if PAGE_ALIAS == 0
template <class Rep, class Ratio>
struct page
{
    using rep = Rep;
    using unit_traits = bytes_unit_traits<Rep, Ratio>;
    using unit_type = bytes_unit<Rep, Ratio>;

    static constexpr int aliasing = Ratio::num;
    static_assert(aliasing % sizeof(void*) == 0, "Aliasing must fall on pointer size boundary");
    static constexpr rep null = unit_traits::default_value();

    constexpr bool is_null() const { return pos_ == null; }
    void reset() { pos_ = null; }

    constexpr const unit_type pos() const { return unit_type{ pos_ }; }
    void pos(unit_type v) { pos_ = v.count(); }

private:
    rep pos_{null};

public:
    friend void swap(page& lhs, page& rhs) noexcept
    {
        rep temp = lhs.pos_;
        lhs.pos_ = rhs.pos_;
        rhs.pos_ = temp;
        //std::swap(lhs.pos_, rhs.pos_);    // Not happy on packed types
    }

}    __attribute__((packed));

#endif


}}

}}
