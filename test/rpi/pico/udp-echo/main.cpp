#include <test/support.h>

void udp_echo_init(void);


int main()
{
    test::v1::init(WIFI_SSID, WIFI_PASSWORD);

    udp_echo_init();

    unsigned counter = 0;

    for(;;)
    {
#if PICO_CYW43_ARCH_POLL
        test::v1::cyw43_poll();
#else
        test::v1::lwip_poll();
        test::v1::sleep();
#endif
    }
}