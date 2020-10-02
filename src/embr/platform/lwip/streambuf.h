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

namespace impl {

template <class TCharTraits>
class pbuf_streambuf_base
{
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;

public:
    typedef Pbuf::size_type size_type;

protected:
    Pbuf pbuf;

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* data() { return static_cast<char_type*>(netbuf.data()); }
    const char_type* data() const { return static_cast<const char_type*>(netbuf.data()); }
#else
    char_type* data() const { return static_cast<char_type*>(pbuf.payload()); }
#endif
    size_type size() const { return pbuf.length(); }

#ifdef FEATURE_CPP_MOVESEMANTIC
        template <class ...TArgs>
        pbuf_streambuf_base(TArgs&&... args) :
                pbuf(std::forward<TArgs>(args)...) {}
#endif

public:
};

template <class TCharTraits>
struct opbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    estd::internal::impl::out_pos_streambuf_base<TCharTraits>
{
    typedef estd::internal::impl::out_pos_streambuf_base<TCharTraits> base_type;
    typedef TCharTraits traits_type;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    typedef Pbuf::pbuf_pointer pbuf_pointer;

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

    int_type xout_avail() const { return this->size() - this->pos(); }

public:
    // NOTE: Not saving to output sequence, because for most of our implementations, this one
    // included, we treat pbase() buffer and output sequence as the same
    int_type overflow(int_type ch = traits_type::eof())
    {
        if(xout_avail() == 0)
        {
            pbuf_pointer next = this->pbuf.pbuf()->next;

            if(next == NULLPTR) return traits_type::eof();

            // DEBT: See below placement new usage
            new (&this->pbuf) Pbuf(next, false);
        }

        if(ch != traits_type::eof())
        {
            // it's presumed that next buf in pbuf chain can fit at least one character
            *pbase() = ch;
        }

        // DEBT: We can do better than this.  Can't return ch since sometimes it's eof
        // even when we do have more buffer space
        return traits_type::eof() - 1;
    }

    int_type sputc(char_type ch)
    {
        int_type _ch = traits_type::to_int_type(ch);
        int_type result = overflow(_ch);
        if(result == traits_type::eof()) return traits_type::eof();
        this->pbump(1);
        return _ch;
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
        template <class ...TArgs>
        opbuf_streambuf(TArgs&&... args) :
                pbuf_streambuf_base<TCharTraits>(std::forward<TArgs>(args)...) {}
#endif
};

template <class TCharTraits>
struct ipbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    estd::internal::impl::in_pos_streambuf_base<TCharTraits>
{
    typedef estd::internal::impl::in_pos_streambuf_base<TCharTraits> base_type;
    typedef TCharTraits traits_type;
    typedef Pbuf::pbuf_pointer pbuf_pointer;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

protected:
    char_type* eback() const { return this->data(); }
    char_type* gptr() const { return eback() + base_type::pos(); }
    char_type* egptr() const { return eback() + this->size(); }

    int_type xin_avail() const { return this->size() - base_type::pos(); }

    char_type xsgetc() const { return *gptr(); }

public:
    int_type underflow()
    {
        pbuf_pointer next = this->pbuf.pbuf()->next;

        if(next == NULLPTR)  traits_type::eof();

        // DEBT: Kind of a cheezy way around a mutator, but not wrong
        // reinitialize our pbuf and don't bump reference since we're moving through
        // existing one as a read-only operation
        new (&this->pbuf) Pbuf(next, false);

        return xsgetc();
    }
};
}

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_opbuf_streambuf = estd::internal::streambuf<impl::opbuf_streambuf<CharTraits> >;

template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_ipbuf_streambuf = estd::internal::streambuf<impl::ipbuf_streambuf<CharTraits> >;

#endif

//typedef embr::mem::out_netbuf_streambuf<char, PbufNetbuf> opbuf_streambuf;
//typedef embr::mem::in_netbuf_streambuf<char, PbufNetbuf> ipbuf_streambuf;


}

}}