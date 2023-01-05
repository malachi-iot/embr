#pragma once

#include <estd/istream.h>
#include "streambuf.h"

namespace embr { namespace lwip {

namespace legacy {

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_ipbufstream = estd::detail::basic_istream<basic_ipbuf_streambuf<CharT, CharTraits> >;
#endif

typedef estd::detail::basic_istream<ipbuf_streambuf> ipbufstream;

}

#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif
namespace upgrading {

typedef estd::detail::basic_istream<embr::lwip::upgrading::ipbuf_streambuf> ipbufstream;

}

}}