// https://gist.github.com/iwanbk/1399729/f252331b6e325a7ae62614ce5da358e474dc7535
// https://lists.nongnu.org/archive/html/lwip-users/2007-06/msg00078.html
// Adapted from PGESP-12

#include "lwip/api.h"
#include "lwip/udp.h"

#include "esp_log.h"

//#define RAW_LWIP_STYLE
#include <embr/platform/lwip/iostream.h>
#include <embr/exp/platform/lwip/transport-udp.h>
#include <embr/exp/platform/lwip/transport-netconn.h>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

#include "process.h"

using namespace embr;
using namespace embr::experimental;

using embr::lwip::PbufBase;
typedef struct pbuf* pbuf_pointer;

void udp_echo_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,  
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_echo_recv";

    if (p != NULL)
    {
        ipbufstream in(p, false);   // will auto-free p since it's not bumping reference
        // FIX: byte at position 16 gets swallowed up
        opbufstream out(16);        // auto chain-grows itspbuf if necessary

        process_out(in, out);

        out.rdbuf()->shrink();

        pbuf_pointer pbuf = out.rdbuf()->pbuf();

        ESP_LOGI(TAG, "pbuf tot_len=%d", pbuf->tot_len);

        udp_sendto(pcb, pbuf, addr, port);
    }
}

void udp_echo_init_netconn()
{
    typedef transport_traits2<netconn, NETCONN_UDP> traits;
    typedef tuple_traits<traits::ibuf_type> tuple_traits;

    traits::transport_type t = traits::create();
    traits::ibuf_type in;

    traits::read(t, &in);
    traits::endpoint_type endpoint = tuple_traits::endpoint(in);

    traits::free(t);
}


void udp_echo_init_pcb()
{
    typedef transport_traits2<udp_pcb> traits;
    typedef tuple_traits<traits::tuple> tuple_traits;
    traits::transport_type t = traits::create();

    traits::tuple in;
    traits::endpoint_type endpoint = tuple_traits::endpoint(in);

/*
    traits::ibuf_type in;

    traits::read(t, &in);
    traits::endpoint_type endpoint = tuple_traits::endpoint(in); */

    traits::free(t);
}


void udp_echo_init2()
{
    typedef Transport<udp_pcb> stream_transport_type;
    
    stream_transport_type ts;

    ts.alloc_and_bind(7);

    static struct fake_context
    {

    } fc;

    // In this case, no closure or even context is actually needed
    ts.read([](stream_transport_type::read_callback_type& rct, fake_context* c)
    {
        //ipbufstream in(rct.streambuf);    // FIX: This should work
        ipbufstream in(rct.in);
        opbufstream out(16);        // auto chain-grows itspbuf if necessary

        process_out(in, out);

        rct.transport().write(*out.rdbuf(), rct.endpoint);

    }, &fc);
}

void udp_echo_init(void)
{
    embr::lwip::udp::Pcb pcb;
    typedef transport_traits<udp_pcb> traits;
    typedef Transport<udp_pcb> stream_transport_type;
    //typedef transport_traits<netconn> netconn_traits;
    //typedef transport_traits_wrapper<netconn_traits, netconn_traits::protocol_type::tcp_ipv4> wrapper_test1;
    //typedef transport_traits_wrapper<netconn_traits, NETCONN_TCP> wrapper_test1;
    transport_traits2<netconn, NETCONN_TCP> wrapper_test1;

    wrapper_test1.got_here();

    auxliary_usage_test2(wrapper_test1);
    
    stream_transport_type ts(embr::lwip::udp::Pcb::create());

    embr::lwip::udp::Pcb p = ts.pcb;

    //opbufstream out(128);

    //traits::write(p, out.rdbuf()->pbuf().pbuf());
    traits::endpoint_transaction transaction;
    transport_results r = traits::begin_write(p, &transaction);

    traits::end_write(p, &transaction);

    // Hmm, danger of stack problems - although in this case no closure means
    // we sidestep it
    traits::read(p, [](const traits::read_callback_type& e)
    {

    });

    traits::begin_read(p, &transaction);

    // get new pcb
    if (!pcb.alloc()) {
        LWIP_DEBUGF(UDP_DEBUG, ("pcb.alloc failed!\n"));
        return;
    }

    /* bind to any IP address on port 7 */
    if (pcb.bind(7) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("pcb.bind failed!\n"));
        return;
    }

    /* set udp_echo_recv() as callback function
       for received packets */
    pcb.recv(udp_echo_recv);
}
