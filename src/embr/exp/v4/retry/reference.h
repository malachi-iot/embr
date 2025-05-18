#pragma once

#include <estd/chrono.h>
#include <estd/internal/fwd/string.h>

#include "fwd.h"

namespace embr { namespace experimental { inline namespace v4 {

template <unsigned N = 128>
struct ReferenceTracked
{
    char buffer_[N];

    estd::layer2::string<N> as_string()
    {
        return { buffer_ };
    }
};

}}}
