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

        pbuf_pointer outgoing_p_test = NULLPTR;

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

            // FIX: better if we exposed non-const netbuf
            outgoing_p_test = const_cast<pbuf_pointer>(netbuf.pbuf());
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

