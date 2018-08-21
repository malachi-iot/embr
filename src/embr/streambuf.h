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

    // Spot for shared netbuf, if we ever actually need it
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
    typedef estd::streamsize streamsize;

private:

    // netbuf represents 'put area' in this context
    TNetbuf netbuf;

    // how far into current netbuf chunk we are
    size_type pos;

    char_type* data() const { return reinterpret_cast<char_type*>(netbuf.data()); }
    size_type size() const { return netbuf.size(); }

public:
    out_netbuf_streambuf(size_type pos = 0) : pos(pos) {}

    template <class TParam1>
    out_netbuf_streambuf(TParam1& p) :
        netbuf(p), pos(0) {}

    // for netbuf flavor we can actually implement these but remember
    // xsputn at will can change all of these values
    char_type* pbase() const { return data(); }
    char_type* pptr() const { return data() + pos; }
    char_type* epptr() const { return data() + size(); }

    // as per documentation, no bounds checking is performed on count
    void pbump(int count) { pos += count; }

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
            bool has_next = netbuf.next();

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
                switch(netbuf.expand(count, true))
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

};


}

#ifdef FEATURE_CPP_ALIASTEMPLATE
template <class CharT, class TNetbuf, class CharTraits = std::char_traits<CharT> >
using netbuf_streambuf = estd::internal::streambuf<impl::out_netbuf_streambuf<CharT, TNetbuf> >;
#endif
}}
