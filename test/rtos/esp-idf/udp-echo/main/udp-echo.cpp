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

typedef embr::lwip::PbufNetbuf netbuf_type;
typedef out_netbuf_streambuf<char, netbuf_type> out_pbuf_streambuf;
typedef in_netbuf_streambuf<char, netbuf_type> in_pbuf_streambuf;
typedef estd::internal::basic_ostream<out_pbuf_streambuf> pbuf_ostream;
typedef estd::internal::basic_istream<in_pbuf_streambuf> pbuf_istream;

//#define RAW_LWIP_STYLE

void process_out(pbuf_istream& in, pbuf_ostream& out)
{
    const char* TAG = "process_out";

    in_pbuf_streambuf& in_rdbuf = *in.rdbuf();
    int tot_len = in_rdbuf.cnetbuf().total_size();

    if(in.peek() == '!')
    {
        in.ignore();
        switch(in.get())
        {
            case '1':
                out << "123";
                break;

            case '2':
                break;

            default:
                break;
        }
    }

    char* inbuf = in_rdbuf.gptr();

    out.write(inbuf, tot_len);
}

void udp_echo_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,  
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_echo_recv";

    if (p != NULL) {
        ESP_LOGI(TAG, "entry: p->len=%d", p->len);

        pbuf_istream in(p);

        // brute force copy
        struct pbuf* outgoing_p =

        // probably making this a PBUF_TRANSPORT is what fixes things
        pbuf_alloc(PBUF_TRANSPORT, p->tot_len, PBUF_RAM);

        // having some serious issues with ref counting
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
#ifdef RAW_LWIP_STYLE
            pbuf_copy(outgoing_p, p);
#else
            // since above seems to be true, scope this so ref count goes back down to 1
            pbuf_ostream out(outgoing_p);

            process_out(in, out);

            //out.rdbuf()->sputn(inbuf, p->len);

            int tot_len_exp = out.rdbuf()->total_size_experimental();
            int num_chains = out.rdbuf()->cnetbuf().chain_counter();

            // FIX: glitch in total_size_experimental, returning 2 too many
            // so when I expect we're outputting 7, we get a count of 9
            ESP_LOGI(TAG, "tot_len_exp = %d", tot_len_exp);
            ESP_LOGI(TAG, "# of chains = %d", num_chains);

            // FIX: exposing way too many innards to achieve this pbuf_realloc
            // however, calling the experimental 'shrink' so far is proving tricky
            // also
            pbuf_realloc(outgoing_p, tot_len_exp);
#endif
        }

        /* send received packet back to sender */
        udp_sendto(pcb, outgoing_p, addr, port);

        pbuf_free(outgoing_p);

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