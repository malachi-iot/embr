#include <estd/chrono.h>
#include <estd/ostream.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>
#include <pico/time.h>

static estd::pico_ostream cout(stdio_usb);

namespace estd {

estd::pico_ostream& clog = cout;

}

using namespace estd;
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

    const ip4_addr_t* addr = nullptr;

    for(;;)
    {
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        sleep_ms(1);

        int new_status = cyw43_tcpip_link_status(
             &cyw43_state,CYW43_ITF_STA);
	
        if((++counter % 1000) == 0)
            cout << "Polled: " << (counter / 1000) << estd::endl;
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        cout << "Background: " << ++counter << estd::endl;
        estd::this_core::sleep_until(now + 1s * counter);
#endif

        if(netif_is_link_up(netif_default) && !ip4_addr_isany_val(*netif_ip4_addr(netif_default)))
        {
            if(addr == nullptr)
            {
                addr = netif_ip4_addr(netif_default);

                char temp[32];
                ip4addr_ntoa_r(addr, temp, sizeof(temp));

                cout << "Got IP: " << temp << endl;
            }
        }
    }
}