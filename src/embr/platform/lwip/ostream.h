#pragma once

#include <estd/ostream.h>
#include "streambuf.h"

namespace embr { namespace lwip {

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_opbufstream = estd::internal::basic_ostream<basic_opbuf_streambuf<CharT, CharTraits> >;
#endif

typedef estd::internal::basic_ostream<opbuf_streambuf> opbufstream;

}}