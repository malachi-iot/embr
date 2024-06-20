#include <esp_log.h>

#include <estd/ostream.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/usb-serial-jtag/streambuf.h>

// DEBT: Make std::chrono work with estd::this_thread
using namespace estd::chrono_literals;

//template <class Streambuf, unsigned N = 64>
//using out_bipbuf_streambuf = detail::streambuf<impl::out_buffered_bipbuf<Streambuf, N> >;

using ocdc_streambuf = embr::esp_idf::ousj_streambuf<char>;

using sb2 = estd::internal::out_bipbuf_streambuf<ocdc_streambuf>;

using ostream = estd::detail::basic_ostream<ocdc_streambuf>;
using ostream2 = estd::detail::basic_ostream<sb2>;

extern "C" void app_main(void)
{
    static const char* TAG = "app_main";

    static auto config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));

    unsigned counter = 0;
    ostream out;
    ostream2 out2;

    for(;;)
    {
        ESP_LOGI(TAG, "counter: %u", counter);

        out << "Hello: " << counter << estd::endl;
        out << "Hello2: " << counter << estd::endl;

        ++counter;

        estd::this_thread::sleep_for(1000ms);
    }
}
