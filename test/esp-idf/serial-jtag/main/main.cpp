#include <estd/ostream.h>

#include "out.h"

using ostream = estd::detail::basic_ostream<ocdc_streambuf>;

extern "C" void app_main(void)
{
    static auto config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));

    ostream out;

    out << "Hello" << estd::endl;
}
