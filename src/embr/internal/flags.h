#pragma once

#include <estd/internal/type_traits.h>

#include "../fwd/flags.h"

namespace embr { namespace experimental {

template <class Enum>
class flags
{
public:
#if FEATURE_ESTD_UNDERLYING_TYPE
    using int_type = estd::underlying_type<Enum>;
#else
    using int_type = int;
#endif

    using value_type = Enum;

private:
    value_type value_;

    // Force int conversion to always fail, we don't want 'bool' conversion to fool us in these cases
    //constexpr operator int_type() const { return value_; }

public:
    constexpr explicit flags(int_type v) :
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

    constexpr flags operator~() const
    {
        return flags(~value_);
    }
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

template <class Enum>
constexpr embr::experimental::flags<Enum> not_helper(const Enum& v)
{
    return embr::experimental::flags<Enum>(Enum(~int(v)));
}


#define EMBR_FLAGS(Enum)    \
constexpr embr::experimental::flags<Enum> operator~(const Enum& v)    \
{ return not_helper(v); }     \
constexpr embr::experimental::flags<Enum> operator|(const Enum& lhs, const Enum& rhs)    \
{ return or_helper(lhs, rhs); }     \
constexpr embr::experimental::flags<Enum> operator&(const Enum& lhs, const Enum& rhs)    \
{ return and_helper(lhs, rhs); }

