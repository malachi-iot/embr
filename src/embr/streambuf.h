#pragma once

#include <estd/streambuf.h>

#include "netbuf.h"

namespace embr { namespace mem {

namespace impl {

template <class TChar, class TNetbuf, class CharTraits>
struct netbuf_streambuf_base
{
    typedef TChar char_type;
    typedef CharTraits traits_type;
    typedef typename estd::remove_reference<TNetbuf>::type netbuf_type;
    typedef typename netbuf_type::size_type size_type;
    typedef estd::streamsize streamsize;

    // Spot for shared netbuf, if we ever actually need it
    // netbuf represents 'put area' in this context
    TNetbuf netbuf;

    char_type* data() { return reinterpret_cast<char_type*>(netbuf.data()); }
    const char_type* data() const { return reinterpret_cast<char_type*>(netbuf.data()); }
    size_type size() const { return netbuf.size(); }

    template <class TParam1>
    netbuf_streambuf_base(TParam1& p) : netbuf(p) {}
};

// EXPERIMENTAL - copy/moved from estd ios branch
// output streambuf whose output destination is a netbuf
// also trying to crowbar internal-pos tracking out of netbuf.  This is a good mechanism for it
// (i.e. 'processed' vs 'unprocessed' netbuf contents)
// note also netbuf architecture is such that it is decoupled from its consumer, so sync/emit
// calls to this streambuf would have diminished meaning unless we also connected this streambuf
// to an actual consuming device somehow.  Not a terrible idea, but not doing that for now
template <class TChar, class TNetbuf,
          class CharTraits = std::char_traits<TChar>,
          class TBase = netbuf_streambuf_base<TChar, TNetbuf, CharTraits> >
struct out_netbuf_streambuf : TBase
{
    typedef TBase base_type;
    typedef TChar char_type;
    typedef CharTraits traits_type;
    typedef typename estd::remove_reference<TNetbuf>::type netbuf_type;
    typedef typename netbuf_type::size_type size_type;
    typedef typename traits_type::int_type int_type;
    typedef estd::streamsize streamsize;

private:

    // how far into current netbuf chunk we are (for put operations)
    size_type pos;

    char_type* data() { return base_type::data(); }
    const char_type* data() const { return base_type::data(); }
    size_type size() const { return base_type::size(); }
    netbuf_type& netbuf() { return base_type::netbuf; }
    const netbuf_type& netbuf() const { return base_type::netbuf; }

    // end of particular chunk has been reached
    bool eol() const { return pos == size(); }

public:
    out_netbuf_streambuf(size_type pos = 0) : pos(pos) {}

    template <class TParam1>
    out_netbuf_streambuf(TParam1& p) :
        base_type(p), pos(0) {}

    // for netbuf flavor we can actually implement these but remember
    // xsputn at will can change all of these values
    char_type* pbase() const { return data(); }
    char_type* pptr() const { return data() + pos; }
    char_type* epptr() const { return data() + size(); }

    // as per documentation, no bounds checking is performed on count
    void pbump(int count) { pos += count; }


    // TODO: We need to figure out our interaction with device itself (i.e. output sequence)
    // which is supposed
    // to empty these things out - probably interacting with overflow() and/or sync()
    // NOTE: Not ready yet because it doesn't do expands
    /*
    int_type sputc(char_type ch)
    {
        if(eol()) return traits_type::eof();

        *pptr() = ch;

        pos++;
    } */

    // NOTE: Duplicated code from elsewhere.  Annoying, but expected
    // since this is the first time I've put it in a truly standard place
    streamsize xsputn(const char_type* s, streamsize count)
    {
        char_type* d = data() + pos;
        streamsize orig_count = count;
        size_type remaining = size() - pos;

        // if we have more to write than fits in the current netbuf.data()
        while(count > remaining)
        {
            // put in as much as we can
            count -= remaining;
            while(remaining--) *d++ = *s++;

            // move to next netbuf.data()
            bool has_next = netbuf().next();

            // whether or not has_next succeeds, pos is reset here
            // this means if it fails, the next write operation will overwrite the contents
            // so keep an eye on your return streamsize
            pos = 0;

            if(has_next)
            {
                // if there's another netbuf.data() for us to move to, get specs for it
                remaining = size();
                d = data();
            }
            else
            {
                // try to expand.  Not all netbufs can or will
                // also auto advance to next buffer
                switch(netbuf().expand(count, true))
                {
                    case ExpandResult::ExpandOKChained:
                        remaining = size();
                        d = data();
                        break;

                    default:
                        // NOTE: pos is left in an invalid state here
                        // otherwise, we aren't able to write everything so return what we
                        // could do
                        return orig_count - count;
                }
            }
        }

        pos += count;

        while(count--) *d++ = *s++;

        return orig_count;
    }
};

template <class TChar, class TNetbuf,
          class CharTraits = std::char_traits<TChar>,
          class TBase = netbuf_streambuf_base<TChar, TNetbuf, CharTraits> >
struct in_netbuf_streambuf : TBase
{
    typedef TBase base_type;
    typedef typename base_type::char_type char_type;
    typedef typename base_type::size_type size_type;
    typedef typename base_type::traits_type traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename base_type::netbuf_type netbuf_type;
    typedef typename base_type::streamsize streamsize;

private:
    // how far into current netbuf chunk we are (for get operations)
    size_type pos;

    char_type* data() const { return base_type::data(); }
    size_type size() const { return base_type::size(); }
    netbuf_type& netbuf() const { return base_type::netbuf; }

    // end of particular chunk has been reached
    bool eol() const { return pos == size(); }

public:
    template <class TParam1>
    in_netbuf_streambuf(TParam1& p) :
        base_type(p),
        pos(0)
    {}

    // for netbuf flavor we can actually implement these but remember
    // xsgetn at will can change all of these values
    char_type* gbase() const { return data(); }
    char_type* gptr() const { return data() + pos; }
    char_type* egptr() const { return data() + size(); }

    // as per documentation, no bounds checking is performed on count
    void gbump(int count) { pos += count; }

    int_type sgetc()
    {
        // TODO: do a next() here in a nonblocking way
        // to try to get at more data if available.  Unclear how to do this
        // right now because sgetc is supposed to not advance the pointer
        if(eol())
        {
            // eol is a special condition where the 'past the end' position
            // sort of maps onto the 'beginning of the next' position, so go
            // ahead and try to bump forward in this scenario
            if(!netbuf().next()) return traits_type::eof();

            pos = 0;
        }

        // we arrive here if we're pointing at non-eol of a valid chunk
        return traits_type::to_int_type(*gptr());
    }

    // TODO: Consolidate this into estd::internal::streambuf as a SFINAE
    // selection if sgetc is available and sbumpc is NOT explicitly implemented
    // in Impl
    int_type sbumpc()
    {
        int_type ret_value = sgetc();

        // if ret_value is not eof, then by definition we have at least
        // one pos we can advance.
        if(ret_value != traits_type::eof()) pos++;

        return ret_value;
    }

    streamsize xsgetn(char_type* d, streamsize count)
    {
        const char_type* s = data() + pos;
        streamsize orig_count = count;
        // remaining = number of bytes available to read out of this chunk
        size_type remaining = size() - pos;

        while(count > remaining)
        {
            count -= remaining;
            while(remaining--) *d++ = *s++;

            pos = 0;

            if(netbuf().next())
            {
                s = data();
                remaining = size();
            }
            else
            {
                // don't try to expand on a read, that would be invalid/synthetic input
                // instead, just report how much we did succeed in reading (basically
                // we are EOF)
                return orig_count - count;
            }
        }

        // we get here when count <= remaining, and don't need
        // to issue a 'next'
        pos += count;
        while(count--) *d++ = *s++;

        return orig_count;
    }
};


}

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class TNetbuf, class CharTraits = std::char_traits<CharT> >
using out_netbuf_streambuf = estd::internal::streambuf<impl::out_netbuf_streambuf<CharT, TNetbuf> >;

template <class CharT, class TNetbuf, class CharTraits = std::char_traits<CharT> >
using in_netbuf_streambuf = estd::internal::streambuf<impl::in_netbuf_streambuf<CharT, TNetbuf> >;
#endif
}}
