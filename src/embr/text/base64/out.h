#pragma once

#include <estd/streambuf.h>

namespace embr { namespace text { inline namespace v1 {

namespace impl {

extern const char base64en[];

// Adaptation of https://github.com/nkolban/esp32-snippets/blob/master/cloud/GCP/JWT/base64url.cpp which
// interestingly was already present (in a very old form) in playground.esp repo as a submodule
template <ESTD_CPP_CONCEPT(estd::concepts::v1::OutStreambuf) Wrapped>
struct out_base64_streambuf : estd::internal::impl::streambuf_base<estd::char_traits<char>>
{
    using base_type = estd::internal::impl::streambuf_base<estd::char_traits<char>>;
    using wrapped_type = estd::remove_reference_t<Wrapped>;

    Wrapped wrapped_;

    using streamsize = estd::streamsize;

    // FIX: Need way to populate this guy, I'm looking at you embr::gl rfc/capabilities
    static constexpr bool has_epptr = false;
    static constexpr bool auto_finalize = false;

    void inc()
    {
        if(++s_ == 3)   s_ = 0;
    }

public:
    using typename base_type::traits_type;
    // DEBT: Something about upper char_type is registering as pure int
    using typename base_type::char_type;
    typedef typename traits_type::off_type off_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::int_type int_type;
    int s_ = 0;
    char_type prev_;

    template <class ...Args>
    constexpr out_base64_streambuf(Args&&...args) :
        wrapped_{std::forward<Args>(args)...}
    {}

    ~out_base64_streambuf()
    {
        // FIX: JWT decoder seems to get upset when finalizing payload
        if(auto_finalize)   finalize();
    }

    wrapped_type& wrapped() { return wrapped_; }

    template <class F>
    static void encode(int s, char_type prev, char_type ch, F&& out)
    {
        switch(s)
        {
        case 0:
            out(base64en[(ch >> 2) & 0x3F]);
            break;

        case 1:
            out(base64en[((prev & 0x3) << 4) + ((ch >> 4) & 0xF)]);
            break;

        case 2:
            out(base64en[((prev & 0xF) << 2) + ((ch >> 6) & 0x3)]);
            out(base64en[ch & 0x3F]);
            break;
        }
    }

    // Final one only
    template <class F>
    static void encode(int s, char_type prev, F&& out, bool pad)
    {
        if(s-- == 0)   s = 2;

        if(s == 0)
        {
            out(base64en[(prev & 0x3) << 4]);
            if(pad)
            {
                out('=');
                out('=');
            }
        }
        else if(s == 1)
        {
            out(base64en[(prev & 0xF) << 2]);
            if(pad) out('=');
        }
    }

    int_type sputc(char_type ch)
    {
        encode(s_, prev_, ch, [&](char_type out){ wrapped_.sputc(out); });
        inc();
        prev_ = ch;
        return {};
    }

    void finalize(bool pad = true)
    {
        encode(s_, prev_, [&](char_type out){ wrapped_.sputc(out); }, pad);
        s_ = 0;
        prev_ = 0;
    }

    streamsize xsputn(const char_type* s, streamsize count)
    {
        const char_type* start = s;
        const char_type* end = s + count;

        if constexpr(has_epptr)
        {
            char_type* pptr = wrapped_.pptr();
            char_type* epptr = wrapped_.epptr();

            while(s < end)
            {
                // -1 because encode sometimes pops out 2 characters
                // NOTE: That may not play nice with sync, so check against s_ too
                if(pptr < epptr - 1)
                {
                    encode(prev_, *s, [&](char_type out){ *pptr++ = out; });
                    prev_ = *s++;
                }
                else
                {
                    if(wrapped_.sync() == -1) return s - start;
                }
            }
        }
        else
        {
            while(s < end)
            {
                sputc(*s++);
            }
        }

        return s - start;
    }
};

}

}}}
