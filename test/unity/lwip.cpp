#include "unit-test.h"

#include "esp_log.h"

#include "lwip/tcpip.h"

#include <embr/platform/lwip/pbuf.h>
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

static constexpr int listener_port = 1000;
static constexpr int reply_listener_port = listener_port + 1;

static void listener(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    const char* TAG = "listener";

    embr::lwip::udp::Pcb pcb1(_pcb);

    embr::lwip::Pbuf incoming_buf(p);
    embr::lwip::Pbuf buf(4);

    buf.put_at(0, incoming_buf.get_at(0) + 10);
    buf.put_at(1, incoming_buf.get_at(1) + 10);

    ESP_LOGI(TAG, "entry");

    pcb1.send(buf, &loopback_addr, reply_listener_port);

    ESP_LOGI(TAG, "exit");
}


static void reply_listener(void* arg, struct udp_pcb* _pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
    const char* TAG = "reply_listener";

    //estd::span<uint8_t, 4>& output = * (estd::span<uint8_t, 4>*) arg;
    uint8_t* output = (uint8_t*) arg;
    
    ESP_LOGI(TAG, "entry");

    embr::lwip::Pbuf incoming_buf(p);

    output[0] = incoming_buf.get_at(0);
    output[1] = incoming_buf.get_at(1);

    ESP_LOGI(TAG, "exit");
}


static void test_basic_loopback()
{
    embr::lwip::udp::Pcb pcb1, pcb2;
    embr::lwip::Pbuf buf(4);

    // DEBT: span broken, default ctor has an issue
    //estd::span<uint8_t, 4> output;
    uint8_t output[4];

    buf.put_at(0, 1);
    buf.put_at(1, 2);

    pcb1.alloc();
    pcb2.alloc();

    pcb1.bind(&loopback_addr, listener_port);
    pcb1.recv(listener);
    pcb2.bind(&loopback_addr, reply_listener_port);
    pcb2.recv(reply_listener, &output);

    // Works the same whether or not TCPIP code lock feature is enabled
    LOCK_TCPIP_CORE();
    pcb1.send(buf, &loopback_addr, listener_port);
    UNLOCK_TCPIP_CORE();

    TEST_ASSERT_EQUAL(11, output[0]);
    TEST_ASSERT_EQUAL(12, output[1]);

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
