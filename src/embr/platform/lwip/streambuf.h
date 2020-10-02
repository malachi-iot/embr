#pragma once

#include "pbuf.h"
#include "../../streambuf.h"

namespace embr { namespace lwip {

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

// NOTE: Not ready for prime time yet.  Will displace all my netbuf abstractions, they are finally
// going away and replaced by more straightforward estd::streambuf capabilities
namespace upgrading {

template <class TCharTraits>
class pbuf_streambuf_base
{
public:
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::size_type size_type;

protected:
    Pbuf pbuf;

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* data() { return static_cast<char_type*>(netbuf.data()); }
    const char_type* data() const { return static_cast<const char_type*>(netbuf.data()); }
#else
    char_type* data() const { return static_cast<char_type*>(pbuf.payload()); }
#endif
    size_type size() const { return pbuf.length(); }

public:
};

template <class TCharTraits>
struct opbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    estd::internal::impl::out_pos_streambuf_base<TCharTraits>
{
    typedef estd::internal::impl::out_pos_streambuf_base<TCharTraits> base_type;
    typedef TCharTraits traits_type;

    using typename base_type::char_type;

protected:
#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* pbase() { return this->data(); }
    const char_type* pbase() const { return this->data(); }
    char_type* pptr() { return pbase() + pos; }
    const char_type* pptr() const { return data() + pos; }
    char_type* epptr() const { return pbase() + size(); }
#else
    char_type* pbase() const { return this->data(); }
    char_type* pptr() const { return pbase() + this->pos(); }
    char_type* epptr() const { return pbase() + this->size(); }
#endif

};

template <class TCharTraits>
struct ipbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    estd::internal::impl::in_pos_streambuf_base<TCharTraits>
{
    typedef estd::internal::impl::in_pos_streambuf_base<TCharTraits> base_type;
    typedef TCharTraits traits_type;

    using typename base_type::char_type;

protected:
    char_type* eback() const { return this->data(); }
    char_type* gptr() const { return eback() + base_type::pos(); }
    char_type* egptr() const { return eback() + this->size(); }
};


}

}}