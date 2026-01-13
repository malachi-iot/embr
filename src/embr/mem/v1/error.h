#pragma once

#include <estd/expected.h>

#if FEATURE_STD_OSTREAM
#include <ostream>
#endif

// DEBT: Really we need FEATURE_STD_STRING_VIEW, estd doesn't provide that yet
#if FEATURE_STD_CHARCONV
#include <string_view>
#else
#include <estd/string_view.h>
#endif

namespace embr { namespace mem { inline namespace v1 {

struct validation_error
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

using validated_result = estd::expected<void, validation_error>;

using invariant_violation = validation_error;

using invariant_result = estd::expected<void, invariant_violation>;

#define EMBR_MEM_INVARIANT_ASSERT(v, rule, details)     \
if((v) == false)    return invariant_result::unexpected_type({rule, details});

#if FEATURE_STD_OSTREAM
template <class Char>
std::basic_ostream<Char>& operator <<(std::basic_ostream<Char>& out, const invariant_result& ir)
{
    if(ir)  return out << "Good";

    return out << "Violation: " << ir.error().rule << ", " << ir.error().details;
}

#endif

}}}
