#pragma once

#include <estd/streambuf.h>

#include <driver/usb_serial_jtag.h>

namespace embr::esp_idf {

// See https://github.com/espressif/esp-idf/blob/v5.2.2/components/driver/usb_serial_jtag/include/driver/usb_serial_jtag.h

namespace impl {

template <class CharTraits>
struct usj_streambuf : estd::internal::impl::streambuf_base<CharTraits>,
    estd::internal::streambuf_sbumpc_tag
{
    using base_type = estd::internal::impl::streambuf_base<CharTraits>;

    using typename base_type::traits_type;
    using typename base_type::char_type;
    using typename base_type::int_type;

    // DEBT: Revisit how streambufs are supposed to do blocking/nonblocking

protected:
/*
    int_type overflow(int_type ch = traits_type::eof())
    {
        return {};
    }   */

public:
    int_type sputc(char_type ch)
    {
        //const int_type _ch = traits_type::to_int_type(ch);
        //int_type result = overflow(_ch);
        int result = usb_serial_jtag_write_bytes(&ch, sizeof(ch), 0);
        //if(result == traits_type::eof()) return traits_type::eof();
        //return _ch;
        return result == 1 ?
            traits_type::to_int_type(ch) :
            traits_type::eof();
    }

    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        return usb_serial_jtag_write_bytes(s, count, 0);
    }

    int_type sbumpc()
    {
        char_type ch;
        int result = usb_serial_jtag_read_bytes(&ch, sizeof(ch), 0);
        return result == 1 ?
            traits_type::to_int_type(ch) :
            traits_type::eof();
    }

    estd::streamsize xsgetn(char_type* s, estd::streamsize count)
    {
        return usb_serial_jtag_read_bytes(s, count, 0);
    }
};

}

template <class Char, class CharTraits = estd::char_traits<char> >
using usj_streambuf = estd::detail::streambuf<
    impl::usj_streambuf<CharTraits> >;

}