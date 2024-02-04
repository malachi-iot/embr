#pragma once

#include <estd/streambuf.h>

#include <esp_http_server.h>

namespace embr { namespace esp_idf { namespace impl {

// TODO: Would be interesting to do an httpd_send variety also

template <class Traits, bool low_level = false>
class httpd_ostreambuf;

template <class Traits>
class httpd_ostreambuf<Traits, false> :
    public estd::internal::impl::streambuf_base<Traits>
{
    using base_type = estd::internal::impl::streambuf_base<Traits>;

    httpd_req_t* req_;

public:
    using typename base_type::int_type;
    using typename base_type::char_type;
    using typename base_type::traits_type;

    constexpr httpd_ostreambuf(httpd_req_t* req) :
        req_{req}
    {}

    // NOTE: Not ideal to call this guy, we'd prefer a wrapping buffer ostreambuf
    // of some kind to absorb character-by-character calls
    // (httpd_resp_send_chunk has notable overhead)
    int_type sputc(char_type ch)
    {
        esp_err_t ret = httpd_resp_send_chunk(req_, &ch, 1);
        return ret == ESP_OK ?
            traits_type::to_int_type(ch) :
            traits_type::eof();
    }

    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        esp_err_t ret = httpd_resp_send_chunk(req_, s, count);
        return ret == ESP_OK ? count : 0;
    }

};

// DEBT: Move this out to estd
template <class Streambuf>
class simple_buffer_ostreambuf :
    public estd::internal::impl::streambuf_base<typename Streambuf::traits_type>
{
    Streambuf streambuf_;

    // TODO: Make this an is-a not a has-a so we can do both istreambuf and ostreambuf
    estd::layer1::string<64, false> buf_;

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(simple_buffer_ostreambuf, streambuf_)
};

}

// DEBT: It's time to liberate streambuf from 'internal', but estd::detail::streambuf might
// imply too much functionality... maybe?
template <class CharT, class CharTraits = estd::char_traits<CharT> >
using basic_httpd_ostreambuf = estd::internal::streambuf<impl::httpd_ostreambuf<CharTraits> >;

template <class CharT, class CharTraits = estd::char_traits<CharT> >
using basic_httpd_ostream = estd::detail::basic_ostream<basic_httpd_ostreambuf<CharT, CharTraits> >;

using httpd_ostream = basic_httpd_ostream<char>;

}}