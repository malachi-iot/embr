#include "unit-test.h"

#include <estd/thread.h>

// DEBT: Re-enable this on Arduino when lwip is present
#if !defined(ARDUINO) && defined(ESP_PLATFORM) || defined(EMBR_PICOW_BOARD)

#include "esp_log.h"

#include "lwip/tcpip.h"

#include <embr/platform/lwip/endpoint.h>
#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/udp.h>

#include <embr/platform/lwip/streambuf.h>
#include <estd/ostream.h>

#if ESTD_OS_FREERTOS
#include <estd/port/freertos/event_groups.h>
#endif

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

// Some help from
// https://stackoverflow.com/questions/76945727/lwip-stack-modified-echo-server-example-sending-data

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

volatile bool spinwait = false;
static estd::layer1::string<32> tcp_buf;

static err_t test_tcp_accept(void* arg, struct tcp_pcb* newpcb, err_t err)
{
    const char* TAG = "test_tcp_accept";

    ESP_LOGI(TAG, "entry");

    // newpcb = "The new connection pcb"

    tcp_pcb_ostream out(newpcb);

    out << "hello";

    out.flush();    // calls pubsync() -> tcp_output()

    ESP_LOGI(TAG, "exit");

    out.rdbuf()->close();

    //tcp_close(newpcb);

    return ERR_OK;
}

static err_t test_tcp_client_recv(void* arg, struct tcp_pcb* _pcb, struct pbuf* p, err_t err)
{
    const char* TAG = "test_tcp_recv";
    embr::lwip::tcp::Pcb pcb(_pcb);

    if(p == nullptr)
    {
        pcb.close();
        pcb.recv(nullptr);  // Not sure this matters
        spinwait = true;
        return ERR_OK;
    }

    embr::lwip::ipbuf_streambuf in(p);
    char* buf = tcp_buf.data();
    int avail = in.in_avail();

    pcb.recved(avail);

    int count = in.sgetn(buf, 32);
    TEST_ASSERT_LESS_THAN(32, count);
    TEST_ASSERT_EQUAL(avail, count);
    // FIX: Not advancing input sequence
    //TEST_ASSERT_EQUAL(0, in.in_avail());

    buf[count] = 0;

    ESP_LOGI(TAG, "p=%p, buf=%s (%d)", p, buf, count);

    pbuf_free(p);

    return ERR_OK;
}


static err_t test_tcp_connected(void* arg, struct tcp_pcb* pcb, err_t err)
{
    const char* TAG = "test_tcp_connected";

    embr::lwip::tcp::Pcb connection(pcb);

    ESP_LOGI(TAG, "entry: pcb=%p, err=%d", pcb, err);

    //connection.arg(nullptr);  doesn't matter
    connection.recv(test_tcp_client_recv);

    // Incomplete type
    //tcp_pcb_istream in(pcb);

    // Needs a pbuf but we don't have that yet
    tcp_pcb_istreambuf in(pcb);

    ESP_LOGI(TAG, "exit");

    return ERR_OK;
}


static void test_tcp_pcb(void *)
{
    embr::lwip::internal::Endpoint<>
        endpoint_client(&loopback_addr, 10000),
        endpoint_server(&loopback2, 80);

    embr::lwip::tcp::Pcb
        // DEBT: Explicitly specifying nullptr as we work out auto_null constructor
        pcb_client(nullptr),
        pcb_server(nullptr);

    pcb_server.create();

    TEST_ASSERT_TRUE(pcb_server.valid());

    pcb_server.bind(endpoint_server);
    TEST_ASSERT_TRUE(pcb_server.listen());
    pcb_server.accept(test_tcp_accept);

    //pcb_server.arg(nullptr);  // doesn't matter

    pcb_client.create();

    TEST_ASSERT_TRUE(pcb_client.valid());

    pcb_client.bind(endpoint_client);
    pcb_client.connect(endpoint_server, test_tcp_connected);

    //return;


    // Now, must wait for connect/accept chain to complete

    TEST_ASSERT_TRUE(pcb_client.valid());
    TEST_ASSERT_TRUE(pcb_server.valid());

    // Don't close yet since we are running on a deferred call
    //pcb_server.close();
    //pcb_client.close();
}


using netconn_nocopy_ostreambuf =
    estd::detail::streambuf<
        embr::lwip::experimental::netconn_nocopy_ostreambuf<
            estd::char_traits<char>
        >
    >;
using netconn_copy_ostreambuf =
    estd::detail::streambuf<
        embr::lwip::experimental::netconn_copy_ostreambuf<
            estd::char_traits<char>
        >
    >;
using netconn_istreambuf =
    estd::detail::streambuf<
        embr::lwip::experimental::netconn_istreambuf<
            estd::char_traits<char>
        >
    >;
using netconn_nocopy_ostream = estd::detail::basic_ostream<netconn_nocopy_ostreambuf>;
using netconn_copy_ostream = estd::detail::basic_ostream<netconn_copy_ostreambuf>;
using netconn_istream = estd::detail::basic_istream<netconn_istreambuf>;

// If we're really lucky, maybe this can upgrade to streambuf_rx_event for system-wide
// goodness
#if ESTD_OS_FREERTOS
estd::freertos::event_group<true> netconn_rx_event;
estd::freertos::event_group<true> netconn_err_event;
#endif


// DEBT: Use estd::intrusive_forward_list once it stabalizes
embr::lwip::experimental::netconn_streambuf_untemplated* base_netconn = nullptr;


// NOTE: This whole notion is based on guidance from
// https://doc.ecoscentric.com/ref/lwip-api-sequential-netconn-new-with-callback.html
// https://lists.gnu.org/archive/html/lwip-devel/2008-02/msg00048.html
// However, LwIP has potentially conflicting instruction:
// https://lwip.fandom.com/wiki/Netconn_API
// It's starting to seem that the meaning of the event changes depending on the operation
static void eval_netconn_callback(
    netconn* nc,
    embr::lwip::experimental::netconn_streambuf_untemplated* n,
    netconn_evt evt)
{
    const char* TAG = "eval_netconn_callback";
    const unsigned rtos_evt = 1 << (n->event_id() - 1);

    ESP_LOGI(TAG, "netconn=%p, rtos_evt = %x", nc, rtos_evt);

#if ESTD_OS_FREERTOS
    if(evt == NETCONN_EVT_RCVMINUS) netconn_rx_event.set_bits(rtos_evt);
    else if(evt == NETCONN_EVT_ERROR) netconn_err_event.set_bits(rtos_evt);
#endif
}

// Re-associates netconn back to our streambuf + stream framework
static void test_netconn_callback(netconn* nc, netconn_evt evt, uint16_t len)
{
    using node_type = embr::lwip::experimental::netconn_streambuf_untemplated*;

    node_type current = base_netconn;

    while(current != nullptr)
    {
        if(current->is_match(nc))
        {
            eval_netconn_callback(nc, current, evt);
            return;
        }

        current = current->next_;
    }
}


static void test_tcp_netconn_nocopy()
{
    embr::lwip::Netconn conn_server, conn_client;

    conn_server.new_with_proto_and_callback(NETCONN_TCP, 0, test_netconn_callback);
    conn_client.new_with_proto_and_callback(NETCONN_TCP, 0, test_netconn_callback);

    conn_server.nonblocking(true);
    // NOTE: Somehow '80' is reserved at this point
    conn_server.bind(81);
    conn_server.listen();

    err_t r = conn_client.connect(&loopback_addr, 81);

    TEST_ASSERT_EQUAL(ERR_OK, r);

    // NOTE: callback is copied from conn_server as per
    // https://www.mail-archive.com/lwip-users@nongnu.org/msg20003.html
    // Also - "we live with having one callback for all types of connections in the socket layer."
    // which I'd put at 80% true, but true enough that it looks like we need one single callback
    // which cascades out through a linked list through all participating streambufs, since
    // there doesn't appear to be an 'arg' parameter
    embr::lwip::Netconn conn_accept;
    r = conn_server.accept(&conn_accept);
    TEST_ASSERT_EQUAL(ERR_OK, r);

    netconn_nocopy_ostream out(conn_accept);
    netconn_istreambuf isb(conn_client);
    //netconn_istream in(conn_client);

    base_netconn = &isb;

    out.write("Hello", 6);
    TEST_ASSERT(out.good());
    //out.flush();

    TEST_ASSERT_EQUAL(0, isb.pubsync());
    TEST_ASSERT_EQUAL(6, isb.in_avail());
    TEST_ASSERT_EQUAL_STRING("Hello", isb.gptr());

#if ESTD_OS_FREERTOS
    // Verify rx bit got set
    EventBits_t rb = netconn_rx_event.wait_bits(1, true, true,
        estd::chrono::milliseconds(100));

    TEST_ASSERT_EQUAL(1, rb);
#endif
}

static void test_tcp_netconn_copy()
{
    embr::lwip::Netconn conn_server, conn_client;

    conn_server.alloc(NETCONN_TCP);
    conn_client.alloc(NETCONN_TCP);

    conn_server.nonblocking(true);
    // NOTE: Somehow '80' is reserved at this point
    conn_server.bind(81);
    conn_server.listen();

    err_t r = conn_client.connect(&loopback_addr, 81);

    TEST_ASSERT_EQUAL(ERR_OK, r);

    embr::lwip::Netconn conn_accept;
    r = conn_server.accept(&conn_accept);
    // FIX
    // ESP32 gives us ERR_WOULDBLOCK
    // ESP32S3 didn't seem to
    TEST_ASSERT_EQUAL(ERR_OK, r);

    netconn_copy_ostream out(conn_accept);
}

static void test_tcp()
{
    test_tcp_netconn_nocopy();
    test_tcp_netconn_copy();

    // Remember tcp_pcb "raw" APIs must be thunked onto tcp task! otherwise
    // nasty crashes occur ... at best
    tcpip_callback(test_tcp_pcb, nullptr);
    //test_tcp(nullptr);

    // Don't spinwait in the thunked function.  If this wasn't a unit test we would use
    // a semaphore etc.

    while(spinwait == false)
    {
        estd::this_thread::sleep_for(estd::chrono::milliseconds(200));
    }

    TEST_ASSERT_TRUE(tcp_buf == "hello");
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
