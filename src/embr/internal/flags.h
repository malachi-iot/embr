#pragma once

#include "../fwd/flags.h"

namespace embr { namespace experimental {

template <class Enum>
class flags
{
    using value_type = Enum;

    value_type value_;

public:
    constexpr flags(const value_type& value) : value_{value}    {}

    constexpr flags& operator|=(const flags& v)
    {
        value_ |= v;
        return *this;
    }

    constexpr operator value_type() const { return value_; }

    constexpr value_type value() const { return value_; }
};


template <class Enum>
constexpr flags<Enum> operator|(const flags<Enum>& lhs, const flags<Enum>& rhs)
{
    return flags<Enum>(Enum(lhs.value() | rhs.value()));
}


template <class Enum>
constexpr flags<Enum> operator|(const flags<Enum>& lhs, const Enum& rhs)
{
    return flags<Enum>(Enum(lhs.value() | rhs));
}


}}
