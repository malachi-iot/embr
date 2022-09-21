#pragma once

#include "base.h"

namespace embr { namespace lwip {

// Displaces all netbuf abstractions, they are finally gone
// and replaced by more straightforward estd::streambuf capabilities
#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif
namespace upgrading {

namespace impl {

template <class TCharTraits>
struct ipbuf_streambuf : 
    pbuf_streambuf_base<TCharTraits>,
    pbuf_current_base<TCharTraits>,
    estd::internal::impl::in_pos_streambuf_base<TCharTraits>
{
    typedef pbuf_streambuf_base<TCharTraits> pbuf_base_type;
    typedef estd::internal::impl::in_pos_streambuf_base<TCharTraits> base_type;
    typedef pbuf_current_base<TCharTraits> pbuf_current_base_type;
    typedef TCharTraits traits_type;

    typedef Pbuf::pbuf_pointer pbuf_pointer;
    typedef Pbuf::size_type size_type;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    char_type* eback() const { return this->current_data(); }
    char_type* gptr() const { return eback() + base_type::pos(); }
    char_type* egptr() const { return eback() + this->current_size(); }

protected:
    bool move_next()
    {
        if(!pbuf_current_base_type::move_next()) return false;

        base_type::seekpos(0);
        return true;
    }


    int_type xin_avail() const { return this->current_size() - base_type::pos(); }

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

            if(!this->move_next()) return written;

            avail = this->current_size();
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
            if(!this->move_next()) return -1;

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
        // DEBT: Technically we need to check xin_avail first
        if(!this->move_next()) return traits_type::eof();

        return xsgetc();
    }

#ifdef __cpp_variadic_templates
        template <class ...TArgs>
        ipbuf_streambuf(TArgs&&... args) :
            pbuf_base_type(std::forward<TArgs>(args)...),
            pbuf_current_base_type(pbuf_base_type::pbuf)
        {
        }
#endif
};

}}

}}