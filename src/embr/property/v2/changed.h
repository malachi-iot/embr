#pragma once

#include "fwd.h"
#include "traits.h"

namespace embr { inline namespace property { inline namespace v2 {

namespace detail {

template <class String>
struct named
{
    String name;
};

}

template <const char* prop, class Enabled>
struct changed
{
    using traits = v2::traits<prop>;
    //static_assert(traits::is_specialized);
    //using value_type = typename traits::value_type;
};


}}}
