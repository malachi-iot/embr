#pragma once

#include <estd/istream.h>
#include "streambuf.h"

namespace embr { namespace lwip {

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_ipbufstream = estd::internal::basic_istream<basic_ipbuf_streambuf<CharT, CharTraits> >;
#endif

typedef estd::internal::basic_istream<ipbuf_streambuf> ipbufstream;

namespace upgrading {

typedef estd::internal::basic_istream<embr::lwip::upgrading::ipbuf_streambuf> ipbufstream;

}

}}