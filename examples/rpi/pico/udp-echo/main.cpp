#include <estd/chrono.h>
#include <estd/ostream.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>
#include <pico/time.h>

static estd::pico_ostream cout(stdio_usb);
static estd::pico_ostream& clog = cout;

using namespace estd::chrono_literals;

typedef estd::chrono::experimental::pico_clock steady_clock;

void udp_echo_init(void);


int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        clog << "failed to initialise" << estd::endl;
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        clog << "failed to connect: ssid=" << WIFI_SSID << ", pass=" << WIFI_PASSWORD << estd::endl;
        return 1;
    }

    udp_echo_init();

    unsigned counter = 0;

    auto now = steady_clock::now();

    for(;;)
    {
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        sleep_ms(1);
        if((++counter % 1000) == 0)
            cout << "Hello Chrono: " << ++counter << estd::endl;
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        cout << "Hello Chrono: " << ++counter << estd::endl;
        estd::this_core::sleep_until(now + 1s * counter);
#endif
    }
}