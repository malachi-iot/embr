#include <esp_log.h>

#include <estd/istream.h>
#include <estd/ostream.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/usb-serial-jtag/streambuf.h>

// DEBT: Make std::chrono work with estd::this_thread
using namespace estd::chrono_literals;

//template <class Streambuf, unsigned N = 64>
//using out_bipbuf_streambuf = detail::streambuf<impl::out_buffered_bipbuf<Streambuf, N> >;

using usj_streambuf = embr::esp_idf::usj_streambuf<char>;

using sb2 = estd::internal::out_bipbuf_streambuf<usj_streambuf>;
using sb3 = estd::internal::in_bipbuf_streambuf<usj_streambuf>;

using ostream = estd::detail::basic_ostream<usj_streambuf>;
using ostream2 = estd::detail::basic_ostream<sb2>;
using istream = estd::detail::basic_istream<usj_streambuf>;
using istream2 = estd::detail::basic_istream<sb3>;

extern "C" void app_main(void)
{
    static const char* TAG = "app_main";

    static auto config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));

    unsigned counter = 0;
    ostream out;
    ostream2 out2;
    istream in;

    for(;;)
    {
        ESP_LOGI(TAG, "counter: %u", counter);

        //int c = in.rdbuf()->sgetc();
        int c = in.rdbuf()->sbumpc();

        out << "Hello: " << counter << estd::endl;
        out2 << "Hello2: " << counter;

        if(c != -1)
            out2 << " (" << char(c) << ')';
        
        out2 << estd::endl;

        ++counter;

        estd::this_thread::sleep_for(1000ms);
    }
}
