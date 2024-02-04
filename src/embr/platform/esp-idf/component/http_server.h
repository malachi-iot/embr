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
    using base_type = estd::internal::impl::streambuf_base<typename Streambuf::traits_type>;

    // TODO: Make this an is-a not a has-a so we can do both istreambuf and ostreambuf
    Streambuf streambuf_;

    static constexpr int len_ = 64;

    // TODO: Make this a string type we pass in somehow
    estd::layer1::string<len_, false> buf_;

    constexpr unsigned remaining() const
    {
        return buf_.max_size() - buf_.size();
    }

    // DEBT: Finish up string/allocated_array underlying full implementation
    constexpr bool full() const
    {
        return remaining() == 0;
    }

    void ll_sync()
    {
        streambuf_.xsputn(buf_.data(), buf_.size());
        buf_.clear();
    }

public:
    using typename base_type::int_type;
    using typename base_type::char_type;
    using typename base_type::traits_type;

protected:
    int sync()
    {
        if(buf_.empty()) return 0;

        ll_sync();

        return 0;
    }

    int_type overflow(int_type ch)
    {
        if(full())
            ll_sync();

        if(traits_type::not_eof(ch))
        {
            buf_ += traits_type::to_char_type(ch);
        }

        return ch;
    }

    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        const estd::streamsize maximum = remaining();

        if(count > maximum) count = maximum;
        
        buf_.append(s, count);

        if(full())
            ll_sync();

        return count;
    }

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(simple_buffer_ostreambuf, streambuf_)

    char_type* pbase() const { return buf_.data(); }
    char_type* pptr() const { return pbase() + buf_.size(); }
    char_type* epptr() const { return pbase() + buf_.max_size(); }

    int_type sputc(char_type ch)
    {
        buf_ += ch;
        return ch;
    }
};

}

// DEBT: It's time to liberate streambuf from 'internal', but estd::detail::streambuf might
// imply too much functionality... maybe?
template <class CharT, class CharTraits = estd::char_traits<CharT> >
using basic_httpd_ostreambuf = estd::internal::streambuf<impl::httpd_ostreambuf<CharTraits> >;

template <class CharT, class CharTraits = estd::char_traits<CharT> >
using basic_httpd_ostream = estd::detail::basic_ostream<basic_httpd_ostreambuf<CharT, CharTraits> >;

using httpd_ostream = basic_httpd_ostream<char>;

template <class Streambuf>
using basic_simple_buffer_ostreambuf = estd::internal::streambuf<
    impl::simple_buffer_ostreambuf<Streambuf> >;

template <class Streambuf>
using buffered_ostream = estd::detail::basic_ostream<
    basic_simple_buffer_ostreambuf<Streambuf> >;

}}