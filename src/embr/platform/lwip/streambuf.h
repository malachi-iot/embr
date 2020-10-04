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
                pbuf(std::forward<TArgs>(args)...)
        {
        }
#endif

public:
};

/*
class pbuf_current_base
{
protected:
    PbufBase pbuf_current;

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* current_data() { return static_cast<char_type*>(netbuf.data()); }
    const char_type* current_data() const { return static_cast<const char_type*>(netbuf.data()); }
#else
    char_type* current_data() const
    { return static_cast<char_type*>(pbuf_current.payload()); }
#endif
    size_type current_size() const { return pbuf.length(); }
}; */

template <class TCharTraits>
struct opbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    estd::internal::impl::out_pos_streambuf_base<TCharTraits>
{
    typedef pbuf_streambuf_base<TCharTraits> pbuf_base_type;
    typedef estd::internal::impl::out_pos_streambuf_base<TCharTraits> base_type;
    typedef TCharTraits traits_type;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    typedef Pbuf::pbuf_pointer pbuf_pointer;
    typedef Pbuf::size_type size_type;

private:
    PbufBase pbuf_current;

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* current_data() { return static_cast<char_type*>(netbuf.data()); }
    const char_type* current_data() const { return static_cast<const char_type*>(netbuf.data()); }
#else
    char_type* current_data() const
    { return static_cast<char_type*>(pbuf_current.payload()); }
#endif
    size_type current_size() const { return pbuf_current.length(); }

public:

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* pbase() { return current_data(); }
    const char_type* pbase() const { return current_data(); }
    char_type* pptr() { return pbase() + pos; }
    const char_type* pptr() const { return pbase() + pos; }
    char_type* epptr() const { return pbase() + current_size(); }
#else
    char_type* pbase() const { return current_data(); }
    char_type* pptr() const { return pbase() + this->pos(); }
    char_type* epptr() const { return pbase() + current_size(); }
#endif

private:
    // amount of buffer space left we can write to for this particular pbuf chain portion
    int_type xout_avail() const { return current_size() - this->pos(); }

public:
    // NOTE: Not saving to output sequence, because for most of our implementations, this one
    // included, we treat pbase() buffer and output sequence as the same
    int_type overflow(int_type ch = traits_type::eof())
    {
        if(xout_avail() == 0)
        {
            pbuf_pointer next = pbuf_current.pbuf()->next;

            if(next == NULLPTR) return traits_type::eof();

            pbuf_current = next;

            // it's presumed that next buf in pbuf chain can fit at least one character
        }

        if(ch != traits_type::eof())
            *pptr() = ch;

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

    // NOTE: Remember, semi-nonstandard, wrapping sputn does all the overflow trickery
    // TODO: Should we be using pbuf_take?  Seems like pbuf_take always does tot_len, which
    //       would make that a no.  But that also would make no sense
    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        int_type avail = xout_avail();
        estd::streamsize remaining = avail - count;
        
        if(remaining < 0)
            count = avail;

        memcpy(pptr(), s, count);
        this->pbump(count);
        return count;
    }

    void shrink()
    {
        this->pbuf.realloc(
            this->pbuf.total_length() - xout_avail());
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    opbuf_streambuf(TArgs&&... args) :
            pbuf_base_type(std::forward<TArgs>(args)...)
    {
        // DEBT: Slightly sloppy way to initialize this.
        // initalizer list preferred
        pbuf_current = pbuf_base_type::pbuf.pbuf();
    }
#endif
};

template <class TCharTraits>
struct ipbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    estd::internal::impl::in_pos_streambuf_base<TCharTraits>
{
    typedef pbuf_streambuf_base<TCharTraits> pbuf_base_type;
    typedef estd::internal::impl::in_pos_streambuf_base<TCharTraits> base_type;
    typedef TCharTraits traits_type;

    typedef Pbuf::pbuf_pointer pbuf_pointer;
    typedef Pbuf::size_type size_type;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

private:
    PbufBase pbuf_current;

    char_type* current_data() const
    { return static_cast<char_type*>(pbuf_current.payload()); }
    size_type current_size() const { return pbuf_current.length(); }
    
    bool move_next()
    {
        pbuf_pointer next = pbuf_current.pbuf()->next;

        if(next == NULLPTR) return false;

        // the pos base type does nothing fancy, it's not absolute pos
        base_type::seekpos(0);

        pbuf_current = next;
        return true;
    }

public:
    char_type* eback() const { return current_data(); }
    char_type* gptr() const { return eback() + base_type::pos(); }
    char_type* egptr() const { return eback() + current_size(); }

protected:
    int_type xin_avail() const { return current_size() - base_type::pos(); }

    char_type xsgetc() const { return *gptr(); }

    estd::streamsize xsgetn(char_type* dest, estd::streamsize count)
    {
        // not using pbuf_copy_partial since it's a little *too* convenient and
        // doesn't tell us what pbuf we leave off on

        int_type avail = xin_avail();
        estd::streamsize written = 0;

        while(count >= avail)
        {
            memcpy(dest, gptr(), avail);
            count -= avail;
            dest += avail;
            written += avail;

            if(!move_next()) return written;

            avail = current_size();
        }

        // only arrive here when count < avail
        if(count > 0)
        {
            memcpy(dest, gptr(), count);
            written += count;
            this->gbump(count);
        }

        return written;
    }

    estd::streamsize showmanyc()
    {
        int_type in_avail = xin_avail();

        if(in_avail == 0)
        {
            if(underflow() == traits_type::eof()) return -1;

            // we trust that the next pbuf we got is always bigger than 0
            in_avail = xin_avail();
        }

        return in_avail;

        /*
         * We don't do total here so that eback, gptr and egptr are usable without any
         * fancy stuff
        pbuf_pointer next = this->pbuf.pbuf()->next;

        while(next != nullptr)
        {
            in_avail += next->len;
            next = 
        } */
    }

public:
    int_type underflow()
    {
        pbuf_pointer next = pbuf_current.pbuf()->next;

        if(next == NULLPTR)  traits_type::eof();

        pbuf_current = next;

        return xsgetc();
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
        template <class ...TArgs>
        ipbuf_streambuf(TArgs&&... args) :
                pbuf_base_type(std::forward<TArgs>(args)...)
        {
            // DEBT: Slightly sloppy way to initialize this.
            // initalizer list preferred
            pbuf_current = pbuf_base_type::pbuf.pbuf();
        }
#endif
};
}

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_opbuf_streambuf = estd::internal::streambuf<impl::opbuf_streambuf<CharTraits> >;

template <class CharT, class CharTraits = std::char_traits<CharT> >
using basic_ipbuf_streambuf = estd::internal::streambuf<impl::ipbuf_streambuf<CharTraits> >;

#endif

typedef basic_opbuf_streambuf<char> opbuf_streambuf;
typedef basic_ipbuf_streambuf<char> ipbuf_streambuf;


}

}}