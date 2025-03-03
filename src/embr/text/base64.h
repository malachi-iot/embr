#pragma once

#include <estd/streambuf.h>

#include "base64/in.h"
#include "base64/out.h"

namespace embr { namespace text { inline namespace v1 {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::OutStreambuf) Wrapped>
using out_base64_streambuf = estd::detail::streambuf<impl::out_base64_streambuf<Wrapped>>;

template <ESTD_CPP_CONCEPT(estd::concepts::v1::InStreambuf) Wrapped>
using in_base64_streambuf = estd::detail::streambuf<impl::in_base64_streambuf<Wrapped>>;


}}}
