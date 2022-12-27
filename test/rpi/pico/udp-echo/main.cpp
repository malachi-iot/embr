#include <estd/chrono.h>
#include <estd/ostream.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>
#include <pico/time.h>

#include <test/support.h>

using namespace estd;
using namespace estd::chrono_literals;

typedef estd::chrono::experimental::pico_clock steady_clock;

void udp_echo_init(void);


int main()
{
    test::v1::init(WIFI_SSID, WIFI_PASSWORD);

    stdio_init_all();

    udp_echo_init();

    unsigned counter = 0;

    auto now = steady_clock::now();

    const ip4_addr_t* addr = nullptr;

    for(;;)
    {
#if PICO_CYW43_ARCH_POLL
        test::v1::cyw43_poll();
	
        if((++counter % 1000) == 0)
            clog << "Polled: " << (counter / 1000) << estd::endl;
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        clog << "Background: " << ++counter << estd::endl;
        estd::this_core::sleep_until(now + 1s * counter);

        test::v1::lwip_poll();
#endif
    }
}