// https://gist.github.com/iwanbk/1399729/f252331b6e325a7ae62614ce5da358e474dc7535
// https://lists.nongnu.org/archive/html/lwip-users/2007-06/msg00078.html
// Adapted from PGESP-12

#include "lwip/api.h"
#include "lwip/udp.h"

void udp_echo_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,  
    const ip_addr_t *addr, u16_t port)
{
    if (p != NULL) {

        // brute force copy
        struct pbuf* copied_p =

        // probably making this a PBUF_TRANSPORT is what fixes things
        pbuf_alloc(PBUF_TRANSPORT, p->tot_len, PBUF_RAM);

        // TODO: Just for testing purposes, do this with our istream/ostream
        pbuf_copy(copied_p, p);

        /* send received packet back to sender */
        udp_sendto(pcb, copied_p, addr, port);

        pbuf_free(copied_p);

        /* free the pbuf */
        pbuf_free(p);
    }
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