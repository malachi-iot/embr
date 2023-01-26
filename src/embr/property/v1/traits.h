#pragma once

#include "fwd.h"

namespace embr { namespace internal {

namespace property { inline namespace v1 {

typedef estd::integral_constant<int, -1> no_numeric_identifier;


// TODO: Consider changing this to property_traits_base
// and/or putting in a new 'properties' namespace
template <class TOwner, class T, int id_ = no_numeric_identifier::value>
struct traits_base :
    embr::property::v1::tag::property_traits,
    embr::property::v1::tag::has_id
{
    typedef TOwner owner_type;
    typedef T value_type;
    static constexpr int id() { return id_; }
    static constexpr const char* name() { return "N/A"; }
};


template <class TOwner, class T>
struct traits_base<TOwner, T, no_numeric_identifier::value> : embr::property::v1::tag::property_traits
{
    typedef TOwner owner_type;
    typedef T value_type;
    static constexpr const char* name() { return "N/A"; }
};


}}

}

namespace property { inline namespace v1 {

// Through lookup mechanism, resolves TOwner + property ID back to property traits
template <class TOwner, int id_>
using PropertyTraits = typename TOwner::id::template lookup<id_>;

}}

}