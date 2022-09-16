#pragma once

#include "../pbuf.h"
#include "../../../streambuf.h"

namespace embr { namespace lwip {

namespace legacy {

// FIX: Doesn't match std type signature.  std uses basic_XXXXbuf for <TChar>
// flavors and XXXXbuf for common type char, so if we did that, ours would look something like:
// out_basic_pbuf_streambuf<char> and out_pbuf_streambuf (no char)
#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_opbuf_streambuf = embr::mem::out_netbuf_streambuf<CharT, PbufNetbuf, CharTraits>;

template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_ipbuf_streambuf = embr::mem::in_netbuf_streambuf<CharT, PbufNetbuf, CharTraits>;

#endif

typedef embr::mem::out_netbuf_streambuf<char, PbufNetbuf> opbuf_streambuf;
typedef embr::mem::in_netbuf_streambuf<char, PbufNetbuf> ipbuf_streambuf;

}

}}