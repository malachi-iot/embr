#include <estd/chrono.h>
#include <estd/string.h>
#include <estd/ostream.h>

#include <embr/platform/lwip/netif.h>

#include <stdio.h>
#include <pico/stdlib.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

#include "test/support.h"

using namespace estd;
using namespace estd::chrono_literals;


namespace test { namespace v1 {

static unsigned counter = 0;

typedef estd::chrono::experimental::pico_clock steady_clock;

void sleep()
{
    static auto now = steady_clock::now();

    // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
    // is done via interrupt in the background. This sleep is just an example of some (blocking)
    // work you might be doing.
    estd::this_core::sleep_until(now + 1s * counter);
    clog << "Background: " << (counter += 5) << estd::endl;
}

/* TODO: Be more fine grained than just 'TString' and build it out here
template <class TString>
void _ip4addr_to_string(TString& s)
{

}
*/

// UNTESTED
estd::layer1::basic_string<char, 32> to_string(const ip4_addr_t* addr)
{
    estd::layer1::basic_string<char, 32> s;

    ip4addr_ntoa_r(addr, s.data(), s.max_size());

    return s;
}


void lwip_poll(embr::lwip::Netif netif)
{
    static const ip4_addr_t* addr = nullptr;

    // TODO: Consider this netif_poll when PICO_CYW43_ARCH_POLL is set,
    // if cyw43_arch_poll isn't doing that for us already
    // netif_poll(netif)

    // Guidance from [3]
    if(addr == nullptr && netif.is_link_up())
    {
        if(!ip4_addr_isany_val(*netif.ip4_addr()))
        {
            addr = netif.ip4_addr();

            char temp[32];
            ip4addr_ntoa_r(addr, temp, sizeof(temp));

            clog << "Got IP: " << temp << endl;
        }
    }
}

void lwip_poll()
{
    lwip_poll(netif_default);
}


#if PICO_CYW43_ARCH_POLL
void cyw43_poll()
{
    // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
    // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
    cyw43_arch_poll();
    sleep_ms(1);

    lwip_poll();

    if((++counter % 5000) == 0)
        clog << "Polled: " << (counter / 1000) << estd::endl;
}
#endif

}}