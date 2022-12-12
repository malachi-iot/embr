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

#include "process.h"

using namespace embr;

using embr::lwip::PbufBase;
typedef struct pbuf* pbuf_pointer;

void udp_echo_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,  
    const ip_addr_t *addr, u16_t port)
{
    static const char* TAG = "udp_echo_recv";

    if (p != NULL)
    {
        ipbufstream in(p, false);   // will auto-free p since it's not bumping reference
        opbufstream out(16);        // auto chain-grows itspbuf if necessary

        process_out(in, out);

        out.rdbuf()->shrink();

        pbuf_pointer pbuf = out.rdbuf()->pbuf();

        ESP_LOGI(TAG, "pbuf tot_len=%d", pbuf->tot_len);

        udp_sendto(pcb, pbuf, addr, port);
    }
}

void udp_echo_init(void)
{
    static const char* TAG = "udp_echo_init";

    embr::lwip::udp::Pcb pcb;

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

    // DEBT: rpi fake ESP_LOGX needs a parameter
    ESP_LOGV(TAG, "successful: listening on port %d", 7);

    /* set udp_echo_recv() as callback function
       for received packets */
    pcb.recv(udp_echo_recv);
}
