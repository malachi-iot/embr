// https://gist.github.com/iwanbk/1399729/f252331b6e325a7ae62614ce5da358e474dc7535
// https://lists.nongnu.org/archive/html/lwip-users/2007-06/msg00078.html
// Adapted from PGESP-12

#include "lwip/api.h"
#include "lwip/udp.h"

#include "esp_log.h"

//#define RAW_LWIP_STYLE
#include <embr/platform/lwip/iostream.h>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

using namespace embr;
using namespace embr::mem;

typedef embr::lwip::PbufNetbuf netbuf_type;
typedef struct pbuf* pbuf_pointer;
#ifdef LEGACY
using embr::lwip::opbufstream;
using embr::lwip::ipbufstream;
#else
using embr::lwip::upgrading::opbufstream;
using embr::lwip::upgrading::ipbufstream;
#endif

//#define RAW_LWIP_STYLE

void process_out(ipbufstream& in, opbufstream& out)
{
    const char* TAG = "process_out";

    //embr::lwip::ipbuf_streambuf& in_rdbuf = *in.rdbuf();
    auto& in_rdbuf = *in.rdbuf();
    //int tot_len = in_rdbuf.cnetbuf().total_size();

    if(in.peek() == '!')
    {
        in.ignore();
        switch(int ch = in.get())
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

    int in_avail = in_rdbuf.in_avail();

    // NOTE: Stack crash on this line sometimes, may need sdkconfig adjustment
    // or perhaps from an lwip callback we are expected to use LWIP_DEBUGF ?
    ESP_LOGD(TAG, "in_avail = %d", in_avail);

    if(in_avail > 0)
    {
        char* inbuf = in_rdbuf.gptr();

        // DEBT: in_avail() does not address input chaining
        out.write(inbuf, in_avail);

        in.ignore(in_avail);
    }
}

void udp_echo_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,  
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_echo_recv";

    typedef netbuf_type::size_type size_type;

    if (p != NULL)
    {
#ifdef LEGACY
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

        ipbufstream in(p, false); // will auto-free p since it's not bumping reference
        opbufstream out(out_len);

        process_out(in, out);

        netbuf_type& netbuf = out.rdbuf()->netbuf();
        pbuf_pointer pbuf = netbuf.pbuf();
        
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
        size_type total_size = out.rdbuf()->absolute_pos();

        ESP_LOGI(TAG, "experimental total_size=%d, p->tot_len=%d", total_size, netbuf.total_size()); 

        // This works pretty well, just something still bugs me about having a direct shrink call
        // in streambuf
        //out.rdbuf()->shrink_to_fit_experimental2();
        
        // low level call, totally acceptable
        pbuf_realloc(pbuf, total_size);

        //netbuf.shrink(total_size);

        ESP_LOGI(TAG, "pbuf tot_len=%d", netbuf.total_size());

#else
        // TODO: Need to make expanding pbuf chain here like we (I think) do above
        size_type out_len = 128;

        ipbufstream in(p, false); // will auto-free p since it's not bumping reference
        opbufstream out(out_len);

        process_out(in, out);

        out.rdbuf()->shrink();

        pbuf_pointer pbuf = out.rdbuf()->pbuf();

        ESP_LOGI(TAG, "pbuf tot_len=%d", pbuf->tot_len);
#endif
        udp_sendto(pcb, pbuf, addr, port);
    }
}

#define USE_AUTOPCB
#ifdef USE_AUTOPCB
void udp_echo_init(void)
{
    embr::lwip::Pcb pcb;

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
#else
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
#endif