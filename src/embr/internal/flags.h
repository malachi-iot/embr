#pragma once

#include "../fwd/flags.h"

namespace embr { namespace experimental {

template <class Enum>
class flags
{
    using value_type = Enum;

    value_type value_;

public:
    constexpr explicit flags(int v) :
        value_{v}
    {}

    constexpr flags(const value_type& value) : value_{value}    {}

    constexpr flags& operator|=(const flags& v)
    {
        value_ |= v;
        return *this;
    }

    constexpr operator value_type() const { return value_; }

    constexpr operator bool() const { return value_ != value_type{}; }

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


template <class Enum>
constexpr embr::experimental::flags<Enum> or_helper(const Enum& lhs, const Enum& rhs)
{
    return embr::experimental::flags<Enum>(Enum(int(lhs) | int(rhs)));
}

template <class Enum>
constexpr embr::experimental::flags<Enum> and_helper(const Enum& lhs, const Enum& rhs)
{
    return embr::experimental::flags<Enum>(Enum(int(lhs) & int(rhs)));
}


#define EMBR_FLAGS(Enum)    \
constexpr embr::experimental::flags<Enum> operator|(const Enum& lhs, const Enum& rhs)    \
{ return or_helper(lhs, rhs); }     \
constexpr embr::experimental::flags<Enum> operator&(const Enum& lhs, const Enum& rhs)    \
{ return and_helper(lhs, rhs); }

