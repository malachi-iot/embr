// https://gist.github.com/iwanbk/1399729/f252331b6e325a7ae62614ce5da358e474dc7535
// https://lists.nongnu.org/archive/html/lwip-users/2007-06/msg00078.html
// Adapted from PGESP-12

#include "lwip/api.h"
#include "lwip/udp.h"

#include "esp_log.h"

//#define RAW_LWIP_STYLE
#include <embr/platform/lwip/pbuf.h>
#include <embr/streambuf.h>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

using namespace embr;
using namespace embr::mem;

typedef out_netbuf_streambuf<char, embr::lwip::PbufNetbuf> out_pbuf_streambuf;
typedef in_netbuf_streambuf<char, embr::lwip::PbufNetbuf> in_pbuf_streambuf;
typedef estd::internal::basic_ostream<out_pbuf_streambuf> pbuf_ostream;
typedef estd::internal::basic_istream<in_pbuf_streambuf> pbuf_istream;

//#define RAW_LWIP_STYLE

void udp_echo_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,  
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_echo_recv";

    if (p != NULL) {
        ESP_LOGI(TAG, "entry: p->len=%d", p->len);

        // brute force copy
        struct pbuf* copied_p =

        // probably making this a PBUF_TRANSPORT is what fixes things
        pbuf_alloc(PBUF_TRANSPORT, p->tot_len, PBUF_RAM);

        // having some serious issues with ref counting
        pbuf_istream in(p);

        // specifically:
        /*
        assertion "p->ref == 1" failed: file "/home/malachi/Projects/ext/esp-idf/components/lwip/lwip/src/core/ipv4/ip4.c", line 889, function: ip4_output_if_opt_src
        abort() was called at PC 0x400d367f on core 1
         */
        // Does this mean ref count must == 1 when issuing a sendto?
        // according to mapped source code,
        // https://github.com/espressif/esp-lwip/blob/3ed39f27981e7738c0a454f9e83b8e5164b7078b/src/core/ipv4/ip4.c
        // it sure seems to.  That's a surprise
        {
            // since above seems to be true, scope this so ref count goes back down to 1
            pbuf_ostream out(copied_p);

#ifdef RAW_LWIP_STYLE
        // TODO: Just for testing purposes, do this with our istream/ostream
            pbuf_copy(copied_p, p);
#else
            char* inbuf = in.rdbuf()->gptr();

            out.write(inbuf, p->len);
            //out.rdbuf()->sputn(inbuf, p->len);
#endif
        }

        /* send received packet back to sender */
        udp_sendto(pcb, copied_p, addr, port);

        pbuf_free(copied_p);

        /* free the pbuf */
        pbuf_free(p);
    }
    else
        ESP_LOGW(TAG, "p == null");
}


void udp_echo_init(void)
{
    struct udp_pcb * pcb;

    /* get new pcb */
    pcb = udp_new();
    if (pcb == NULL) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_new failed!\n"));
        return;
    }

    /* bind to any IP address on port 7 */
    if (udp_bind(pcb, IP_ADDR_ANY, 7) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_bind failed!\n"));
        return;
    }

    /* set udp_echo_recv() as callback function
       for received packets */
    udp_recv(pcb, udp_echo_recv, NULL);
}