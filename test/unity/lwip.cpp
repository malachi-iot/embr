#include "unit-test.h"

// DEBT: Re-enable this on Arduino when lwip is present
#if !defined(ARDUINO) && defined(ESP_PLATFORM) || defined(EMBR_PICOW_BOARD)

#include "esp_log.h"

#include "lwip/tcpip.h"

#include <embr/platform/lwip/endpoint.h>
#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/udp.h>

#include <embr/platform/lwip/streambuf.h>
#include <estd/ostream.h>

// If LwIP loopback capability is present, then consider enabling our loopback tests
#if LWIP_HAVE_LOOPIF && LWIP_LOOPBACK_MAX_PBUFS
#ifndef FEATURE_EMBR_LWIP_LOOPBACK_TESTS
#define FEATURE_EMBR_LWIP_LOOPBACK_TESTS 1
#endif
#endif

static ip_addr_t
    loopback_addr,
    loopback2;        // a different instance, for deeper equality testing


// NOTE: Always be sure to set ipv4 vs ipv6 type, otherwise comparisons
// (and likely other operations) will fail
static void setup()
{
    ip4_addr_set_loopback(ip_2_ip4(&loopback_addr));
    IP_SET_TYPE(&loopback_addr, IPADDR_TYPE_V4);

    ip_addr_set_loopback(false, &loopback2);    // shorthand for above
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

// It seems these only get defined at all if an RTOS is involved
#if NO_SYS
#define LOCK_TCPIP_CORE()
#define UNLOCK_TCPIP_CORE()
#endif

static void test_basic_loopback()
{
    embr::experimental::Unique<embr::lwip::udp::Pcb>
        pcb1, pcb2;

    embr::lwip::Pbuf buf(4);

    // DEBT: span broken, default ctor has an issue
    //estd::span<uint8_t, 4> output;
    uint8_t output[4];

    buf.put_at(0, 1);
    buf.put_at(1, 2);

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
}


static void test_rtos_loopback()
{
    TEST_ASSERT_EQUAL(127, ip4_addr1(ip_2_ip4(&loopback_addr)));
    TEST_ASSERT_EQUAL(0, ip4_addr2(ip_2_ip4(&loopback_addr)));

    TEST_ASSERT_EQUAL(127, ip4_addr1(ip_2_ip4(&loopback2)));
    TEST_ASSERT_EQUAL(0, ip4_addr2(ip_2_ip4(&loopback2)));
}

static void test_endpoint_configuration()
{
    embr::lwip::internal::Endpoint<> endpoint1(&loopback_addr, 10000);
    
    TEST_ASSERT_EQUAL(IPADDR_TYPE_V4, endpoint1.type());
}

static void test_endpoint_equality()
{
    embr::lwip::internal::Endpoint<>
        endpoint1(&loopback_addr, 10000),
        endpoint2(&loopback_addr, 10000),
        endpoint3(&loopback_addr, 10001),

        endpoint_instance(&loopback2, 10000);

    embr::lwip::internal::Endpoint<false>
        endpoint_instance2(&loopback_addr, 10000);

    TEST_ASSERT_TRUE(endpoint1 == endpoint2);
    TEST_ASSERT_FALSE(endpoint1 == endpoint3);

    TEST_ASSERT_TRUE(ip_addr_cmp(&loopback2, &loopback_addr));

    TEST_ASSERT_TRUE(endpoint1 == endpoint_instance);
    TEST_ASSERT_TRUE(endpoint1 == endpoint_instance2);
    
    // TODO
    //TEST_ASSERT_TRUE(endpoint1 != endpoint3);
}

using tcp_pcb_ostreambuf =
    estd::detail::streambuf<
        embr::lwip::experimental::tcp_pcb_ostreambuf<
            estd::char_traits<char>
        >
    >;
using tcp_pcb_istreambuf =
    estd::detail::streambuf<
        embr::lwip::experimental::tcp_pcb_istreambuf<
            estd::char_traits<char>
        >
    >;
using tcp_pcb_ostream = estd::detail::basic_ostream<tcp_pcb_ostreambuf>;
using tcp_pcb_istream = estd::detail::basic_istream<tcp_pcb_istreambuf>;

static err_t test_tcp_accept(void* arg, struct tcp_pcb* newpcb, err_t err)
{
    // newpcb = "The new connection pcb"

    tcp_pcb_ostream out(newpcb);

    out << "hello";

    return ERR_OK;
}

static err_t test_tcp_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err)
{
    return ERR_OK;
}


static err_t test_tcp_connected(void* arg, struct tcp_pcb* pcb, err_t err)
{
    // Incomplete type
    //tcp_pcb_istream in(pcb);

    // Needs a pbuf but we don't have that yet
    tcp_pcb_istreambuf in(pcb);

    return ERR_OK;
}

static void test_tcp()
{
    embr::lwip::internal::Endpoint<>
        endpoint_client(&loopback_addr, 10000),
        endpoint_server(&loopback2, 80);

    embr::lwip::tcp::Pcb pcb_client, pcb_server;

    pcb_server.create();
    pcb_server.bind(endpoint_server);
    pcb_server.listen(10);
    pcb_server.accept(test_tcp_accept);

    pcb_client.create();
    pcb_client.bind(endpoint_client);
    pcb_client.connect(endpoint_server, test_tcp_connected);

    // Now, must wait for connect/accept chain to complete

    pcb_server.close();
    pcb_client.close();
}

#ifdef ESP_IDF_TESTING
TEST_CASE("lwip wrapper / general tests", "[lwip]")
#else
void test_lwip()
#endif
{
    setup();

    RUN_TEST(test_basic_loopback);
    RUN_TEST(test_endpoint_configuration);
    RUN_TEST(test_rtos_loopback);
    RUN_TEST(test_endpoint_equality);
    RUN_TEST(test_tcp);
}

#endif