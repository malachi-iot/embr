#include "unit-test.h"

#include "lwip/tcpip.h"

#include <embr/platform/lwip/udp.h>

// If LwIP loopback capability is present, then consider enabling our loopback tests
#if LWIP_HAVE_LOOPIF && LWIP_LOOPBACK_MAX_PBUFS
#ifndef FEATURE_EMBR_LWIP_LOOPBACK_TESTS
#define FEATURE_EMBR_LWIP_LOOPBACK_TESTS 1
#endif
#endif

static ip_addr_t loopback_addr;

static void setup()
{
#if FEATURE_EMBR_LWIP_LOOPBACK_TESTS
    ip4_addr_set_loopback(ip_2_ip4(&loopback_addr));
#endif
}

static void listener(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{

}


static void reply_listener(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{

}


static void test_basic_loopback()
{
    embr::lwip::udp::Pcb pcb1, pcb2;

    pcb1.alloc();
    pcb2.alloc();

    pcb1.bind(&loopback_addr, 1000);
    pcb1.recv(listener);
    pcb2.bind(&loopback_addr, 1001);
    pcb2.recv(reply_listener);

    pcb2.free();
    pcb1.free();
}


static void test_rtos_loopback()
{

}

#ifdef ESP_IDF_TESTING
TEST_CASE("lwip wrapper / general tests", "[lwip]")
#else
void test_lwip()
#endif
{
    setup();

    RUN_TEST(test_basic_loopback);
    RUN_TEST(test_rtos_loopback);
}
