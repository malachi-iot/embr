#pragma once

#include <estd/streambuf.h>

namespace embr { namespace text { inline namespace v1 { namespace impl {

extern const signed char base64de[];

// Not implemented yet
template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Wrapped>
struct in_base64_streambuf;

}}}}
