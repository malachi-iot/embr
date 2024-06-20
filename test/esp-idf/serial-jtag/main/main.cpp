#include <esp_log.h>

#include <estd/ostream.h>
#include <estd/thread.h>

#include "out.h"

// DEBT: Make std::chrono work with estd::this_thread
using namespace estd::chrono_literals;
using ostream = estd::detail::basic_ostream<ocdc_streambuf>;

extern "C" void app_main(void)
{
    static const char* TAG = "app_main";

    static auto config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));

    unsigned counter = 0;
    ostream out;

    for(;;)
    {
        ESP_LOGI(TAG, "counter: %u", counter);

        out << "Hello: " << counter << estd::endl;

        ++counter;

        estd::this_thread::sleep_for(1000ms);
    }
}
