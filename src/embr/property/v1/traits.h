#pragma once

#include "fwd.h"

namespace embr { namespace internal {

namespace property { inline namespace v1 {

// TODO: Consider changing this to property_traits_base
// and/or putting in a new 'properties' namespace
template <class TOwner, class T, int id_>
struct traits_base : embr::property::v1::tag::property_traits
{
    typedef TOwner owner_type;
    typedef T value_type;
    static constexpr int id() { return id_; }
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