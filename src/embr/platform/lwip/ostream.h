#pragma once

#include <estd/ostream.h>
#include "streambuf.h"

namespace embr { namespace lwip {

#if FEATURE_EMBR_NETBUF_STREAMBUF
namespace legacy {

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_opbufstream = estd::detail::basic_ostream<basic_opbuf_streambuf<CharT, CharTraits> >;
#endif

typedef estd::detail::basic_ostream<opbuf_streambuf> opbufstream;

}
#endif

#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif
namespace upgrading {

typedef estd::detail::basic_ostream<embr::lwip::upgrading::opbuf_streambuf> opbufstream;

}

}}