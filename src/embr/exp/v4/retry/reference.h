#pragma once

#include <estd/chrono.h>

#include "fwd.h"

namespace embr { namespace experimental { inline namespace v4 {

template <unsigned N = 128>
struct ReferenceTracked
{
    char buffer_[N];
};

}}}
