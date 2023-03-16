#pragma once

#include "streambuf/ipbuf.h"
#include "streambuf/opbuf.hpp"

namespace embr { namespace lwip {

// Displaces all netbuf abstractions, they are finally gone
// and replaced by more straightforward estd::streambuf capabilities
#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif
namespace upgrading {

#ifdef __cpp_alias_templates
template <class CharT, class CharTraits = estd::char_traits<CharT> >
using basic_opbuf_streambuf = estd::internal::streambuf<impl::opbuf_streambuf<CharTraits> >;

template <class CharT, class CharTraits = estd::char_traits<CharT> >
using basic_ipbuf_streambuf = estd::internal::streambuf<impl::ipbuf_streambuf<CharTraits> >;

typedef basic_opbuf_streambuf<char> opbuf_streambuf;
typedef basic_ipbuf_streambuf<char> ipbuf_streambuf;
#endif



}

}}