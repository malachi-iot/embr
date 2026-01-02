#pragma once

#include <estd/expected.h>

// DEBT: Really we need FEATURE_STD_STRING_VIEW, estd doesn't provide that yet
#if FEATURE_STD_CHARCONV
#include <string_view>
#else
#include <estd/string_view.h>
#endif

namespace embr { namespace mem {

struct invariant_violation
{
#if FEATURE_STD_CHARCONV
    // So that debugger can see these more easily
    std::string_view rule;
    std::string_view details;
#else
    estd::string_view rule;
    estd::string_view details;
#endif
};

using invariant_result = estd::expected<void, invariant_violation>;

#define EMBR_MEM_INVARIANT_ASSERT(v, rule, details)     \
if((v) == false)    return invariant_result::unexpected_type({rule, details});

}}
