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
    //int tot_len = in_rdbuf.cnetbuf().total_size();

    if(in.peek() == '!')
    {
        in.ignore();
        switch(char ch = in.get())
        {
            case '1':
                out << "123";
                break;

            default:
                out << '!';
                out.put(ch);
                break;
        }
    }

    char* inbuf = in_rdbuf.gptr();
    int in_avail = in_rdbuf.in_avail();

    ESP_LOGD(TAG, "in_avail = %d", in_avail);

    // DEBT: in_avail() does not address input chaining
    out.write(inbuf, in_avail);

    in.ignore(in_avail);
}

void udp_echo_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,  
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_echo_recv";

    typedef netbuf_type::size_type size_type;

    if (p != NULL)
    {
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        // NOTE: We must place this out in a temporary "variable" or use size_type() around it
        // due to https://stackoverflow.com/questions/40690260/undefined-reference-error-for-static-constexpr-member
        // this is because min takes const T&, which semi-demands an address
        size_type constexpr threshold_size = netbuf_type::threshold_size;
        size_type out_len = estd::min(p->tot_len, threshold_size);
#else
        size_type out_len = p->tot_len;
#endif

        ESP_LOGI(TAG, "entry: p->len=%d, out_len=%d", p->len, out_len);

        pbuf_istream in(p, false); // will auto-free p since it's not bumping reference
        pbuf_ostream out(out_len);

        process_out(in, out);

        // Not doing const flavor because we're experimenting with "shrink" call
        //netbuf_type& netbuf = out.rdbuf()->netbuf();
        // Must do const flavor at the moment as netbuf() is private
        const netbuf_type& netbuf = out.rdbuf()->cnetbuf();
        
        /*
        int total_size = netbuf.total_size();
        int size = netbuf.size();
        int pos = out.rdbuf()->pos();

        total_size -= size;
        total_size += pos;

        ESP_LOGI(TAG, "experimental total_size=%d, pos=%d, size=%d", 
            total_size,
            pos,
            size);

        out.rdbuf()->shrink_to_fit_experimental(); */

        // Less bothered to have a total_size call than a shrink call in streambuf
        size_type total_size = out.rdbuf()->total_size_experimental2();

        ESP_LOGI(TAG, "experimental total_size=%d, p->tot_len=%d", total_size, netbuf.total_size()); 

        // This works pretty well, just something still bugs me about having a direct shrink call
        // in streambuf
        //out.rdbuf()->shrink_to_fit_experimental2();
        
        // low level call, totally acceptable
        pbuf_realloc(netbuf, total_size);

        //netbuf.shrink(total_size);

        ESP_LOGI(TAG, "pbuf tot_len=%d", netbuf.total_size());

        udp_sendto(pcb, netbuf, addr, port);
    }
}

void udp_echo_recv_old(void *arg, 
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

        struct pbuf* outgoing_p_test = NULLPTR;

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

            const netbuf_type& netbuf = out.rdbuf()->cnetbuf();
            int tot_len_exp = out.rdbuf()->total_size_experimental();
            int num_chains = netbuf.chain_counter();

            ESP_LOGI(TAG, "tot_len_exp = %d", tot_len_exp);
            ESP_LOGI(TAG, "# of chains = %d", num_chains);

            // FIX: exposing way too many innards to achieve this pbuf_realloc
            // however, calling the experimental 'shrink' so far is proving tricky
            // also
            //pbuf_realloc(outgoing_p, tot_len_exp);
            out.rdbuf()->shrink_to_fit_experimental();

            outgoing_p_test = netbuf.pbuf();
#endif
        }

        /* send received packet back to sender */
        udp_sendto(pcb, outgoing_p_test, addr, port);

        pbuf_free(outgoing_p_test);

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