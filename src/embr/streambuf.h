#pragma once

#include <estd/streambuf.h>

#include "netbuf.h"

// At time of writing, FEATURE_ESTD_IOSTREAM_STRICT_CONST is invented.  It's more experimental, since it's not
// fully functional.  The idea is that additional const-ness than stock std performs for the pbase, pptr, etc.
// could be useful.  This may have additional consequence when we get into more advanced memory buffers which
// can't be const'd up as much (virtual memory, etc - any memory which has side effects when utilizing it)

namespace embr { namespace mem {

namespace impl {

// TODO: move the pos_streambuf_base to instead be double-inherited from the other in/out so we can
// use in_pos_streambuf base etc
template <class TNetbuf, class CharTraits>
struct netbuf_streambuf_base
{
    typedef CharTraits traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename estd::remove_reference<TNetbuf>::type netbuf_type;
    typedef typename netbuf_type::size_type size_type;
    typedef estd::streamsize streamsize;

    TNetbuf netbuf;

#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* data() { return static_cast<char_type*>(netbuf.data()); }
    const char_type* data() const { return static_cast<const char_type*>(netbuf.data()); }
#else
    char_type* data() const { return static_cast<char_type*>(netbuf.data()); }
#endif
    size_type size() const { return netbuf.size(); }

    template <class TParam1>
    netbuf_streambuf_base(TParam1& p) : 
        netbuf(p) {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    netbuf_streambuf_base(TArgs&&... args) :
        netbuf(std::forward<TArgs>(args)...) {}

    netbuf_streambuf_base(netbuf_type&& netbuf) :
        netbuf(std::move(netbuf)) {}
#endif

protected:
    // resets netbuf and then repositions back to the current chunk/chain pos
    // counting how far in the current chunk/chain represents in absolute pos
    pos_type absolute_finder()
    {
        pos_type position = 0;
        char_type* current = data();
        netbuf.reset();

        while(current != data())
        {
            position += size();
            netbuf.next();
        }

        return position;
    }

    pos_type seekoffhelper(pos_type new_pos)
    {
        bool has_next = true;

        // FIX: handle pos past end somehow
        while(new_pos > size() && has_next)
        {
            size_type sz = size();
            has_next = netbuf.next();

            new_pos -= sz;
        }

        return new_pos;
    }    
};

// output streambuf whose output destination is a netbuf
// also trying to crowbar internal-pos tracking out of netbuf.  This is a good mechanism for it
// (i.e. 'processed' vs 'unprocessed' netbuf contents)
// note also netbuf architecture is such that it is decoupled from its consumer, so sync/emit
// calls to this streambuf would have diminished meaning unless we also connected this streambuf
// to an actual consuming device somehow.  Not a terrible idea, but not doing that for now
template <class TChar, class TNetbuf,
          class CharTraits = std::char_traits<TChar>,
          class TBase = netbuf_streambuf_base<TNetbuf, CharTraits> >
struct out_netbuf_streambuf : 
    estd::internal::impl::out_pos_streambuf_base<CharTraits>,
    TBase
{
    typedef TBase base_type;
    typedef estd::internal::impl::out_pos_streambuf_base<CharTraits> out_pos_base_type;
    typedef TChar char_type;
    typedef CharTraits traits_type;
    typedef typename estd::remove_reference<TNetbuf>::type netbuf_type;
    typedef typename netbuf_type::size_type size_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::off_type off_type;
    typedef typename traits_type::pos_type pos_type;
    typedef estd::streamsize streamsize;
    typedef estd::ios_base ios_base;

    // FIX: ugly naming
    const netbuf_type& cnetbuf() const { return base_type::netbuf; }

public:
    size_type size() const { return base_type::size(); }
    size_type pos() const { return out_pos_base_type::pos(); }

    // needed for pbuf shrink (realloc) operation
    size_type total_size_experimental()
    {
        size_type absolute_pos = base_type::absolute_finder();

        return absolute_pos + pos();
    }

private:
#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* data() { return base_type::data(); }
    const char_type* data() const { return base_type::data(); }
#else
    char_type* data() const { return base_type::data(); }
#endif

    netbuf_type& netbuf() { return base_type::netbuf; }

    // end of particular chunk has been reached
    bool eol() const { return pos() == size(); }

protected:
    void pos(size_type p) { out_pos_base_type::pos(p); }

    pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which)
    {
        if(!(which & ios_base::out)) return -1;

        switch(way)
        {
            case ios_base::beg:
                netbuf().reset();
                if(off < size())
                    this->pbump(off);
                else
                {
                    pos(base_type::seekoffhelper(off));
                }
                break;

            case ios_base::cur:
                if(off + pos() < size())
                    this->pbump(off);
                else
                {
                    pos(base_type::seekoffhelper(off + pos()));
                }
                break;

            case ios_base::end:
                // UNTESTED
                while(netbuf().next()) {}
                pos(size() + off);
                break;
        }

        // FIX: not adequate, need to compensate for movement over netbuf chains
        return pos();
    }


public:
    //out_netbuf_streambuf(size_type pos = 0) : pos(pos) {}

    template <class TParam1>
    out_netbuf_streambuf(TParam1& p) :
        base_type(p) {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    out_netbuf_streambuf(TArgs&&... args) :
        base_type(std::forward<TArgs>(args)...) {}

    out_netbuf_streambuf(netbuf_type&& netbuf) :
        base_type(std::move(netbuf)) {}
#endif

    // for netbuf flavor we can actually implement these but remember
    // xsputn at will can change all of these values
#ifdef FEATURE_ESTD_IOSTREAM_STRICT_CONST
    char_type* pbase() { return data(); }
    const char_type* pbase() const { return data(); }
    char_type* pptr() { return data() + pos; }
    const char_type* pptr() const { return data() + pos; }
    char_type* epptr() const { return data() + size(); }
#else
    char_type* pbase() const { return data(); }
    char_type* pptr() const { return data() + pos(); }
    char_type* epptr() const { return data() + size(); }
#endif

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
        char_type* d = pptr();
        streamsize orig_count = count;
        size_type remaining = size() - pos();

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
            pos(0);

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

        this->pbump(count);

        while(count--) *d++ = *s++;

        return orig_count;
    }

    void shrink_to_fit_experimental()
    {
        netbuf().shrink(total_size_experimental());
    }
};

template <class TChar, class TNetbuf,
          class CharTraits = std::char_traits<TChar>,
          class TBase = netbuf_streambuf_base<TNetbuf, CharTraits> >
struct in_netbuf_streambuf : 
    estd::internal::impl::in_pos_streambuf_base<CharTraits>,
    TBase
{
    typedef TBase base_type;
    typedef estd::internal::impl::in_pos_streambuf_base<CharTraits> in_pos_base_type;
    typedef typename base_type::char_type char_type;
    typedef typename base_type::size_type size_type;
    typedef typename base_type::traits_type traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;
    typedef typename base_type::netbuf_type netbuf_type;
    typedef typename base_type::streamsize streamsize;
    typedef estd::ios_base ios_base;

    pos_type pos() const { return in_pos_base_type::pos(); }

    // FIX: ugly naming
    const netbuf_type& cnetbuf() const { return base_type::netbuf; }

private:
    char_type* data() const { return base_type::data(); }
    size_type size() const { return base_type::size(); }

    netbuf_type& netbuf() { return base_type::netbuf; }

    // end of particular chunk has been reached
    bool eol() const { return pos() == size(); }

protected:
    void pos(pos_type p) { in_pos_base_type::pos(p); }

    // TODO: Try to consolidate this into a netbuf_streambuf_base - impediment
    // is that that one doesn't implement the pos base
    pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which)
    {
        // openmode MUST include 'out' in this instance, otherwise error or ignore
        if(!(which & ios_base::in)) return -1;

        switch(way)
        {
            case ios_base::cur:
                if(off + pos() < size())
                    this->gbump(off);
                else
                {
                    pos(base_type::seekoffhelper(off + pos()));
                }
                break;

            case ios_base::beg:
                netbuf().reset();
                if(off < size())
                    pos(off);
                else
                {
                    pos(base_type::seekoffhelper(off));
                }
                break;

            case ios_base::end:
                // UNTESTED
                while(netbuf().next()) {}
                pos(size() + off);
                break;
        }

        // TODO: after doing above legwork, this pos() will be more complex
        return pos();
    }

    // remember, 'underflow' does not advance character forward and only moves
    // netbuf forward if current buffer is exhausted
    // Interesting.... I rewrote sgetc...
    int_type underflow()
    {
        if(eol())
        {
            // if we can't get anything more out of our netbuf
            if(!netbuf().next())
                // return eof.  If netbuf can't provide us any further data, we're done
                return traits_type::eof();

            pos(0);
        }

        // otherwise, yank out current character (without advancing)
        return traits_type::to_int_type(*gptr());
    }

public:
    template <class TParam1>
    in_netbuf_streambuf(TParam1& p) :
        base_type(p)
    {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    in_netbuf_streambuf(TArgs&&... args) :
        base_type(std::forward<TArgs>(args)...) {}

    in_netbuf_streambuf(netbuf_type&& netbuf) :
        base_type(std::move(netbuf)) {}
#endif

    // for netbuf flavor we can actually implement these but remember
    // xsgetn at will can change all of these values
    char_type* eback() const { return data(); }
    char_type* gptr() const { return data() + pos(); }
    char_type* egptr() const { return data() + size(); }

    int_type sgetc()
    {
        return underflow();
        // keep the follow pre-underflow version around since it has good comments
#ifdef UNUSED
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
#endif
    }

/*
    // TODO: Consolidate this into estd::internal::streambuf as a SFINAE
    // selection if sgetc is available and sbumpc is NOT explicitly implemented
    // in Impl.
    // NOTE: Commented, but keeping here since this implementation slightly
    // better than what's in estd::internal::streambuf
    int_type sbumpc()
    {
        int_type ret_value = sgetc();

        // if ret_value is not eof, then by definition we have at least
        // one pos we can advance.
        if(ret_value != traits_type::eof()) this->gbump(1);;

        return ret_value;
    } */

    streamsize xsgetn(char_type* d, streamsize count)
    {
        const char_type* s = gptr();
        streamsize orig_count = count;
        // remaining = number of bytes available to read out of this chunk
        size_type remaining = size() - pos();

        while(count > remaining)
        {
            count -= remaining;
            d = estd::copy_n(s, remaining, d);

            pos(0);

            // NOTE: Consider 'underflow' here since count > remaining means we always have at least 1
            // more character to read here
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
        this->gbump(count);

        estd::copy_n(s, count, d);

        return orig_count;
    }

    streamsize showmanyc()
    {
        // FIX: What we'll need to do for showmanyc in a netbuf is:
        // a) know what our meta-position is, across chained netbufs
        // b) subtract *that* from total_size()
        return netbuf().total_size() - pos();
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
