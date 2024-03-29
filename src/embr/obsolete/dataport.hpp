#include "dataport.h"

namespace embr {

template <class TDatapump, class TTransportDescriptor, class TSubject>
void DatapumpSubject<TDatapump, TTransportDescriptor, TSubject>::service()
{
    // anything queued from transport in? (for application processing)
    if(!datapump.dequeue_empty())
    {
        item_t& item = datapump.dequeue_front();

        notify(typename event::receive_dequeuing(item));

#ifndef FEATURE_EMBR_DATAPUMP_INLINE
        typedef typename datapump_t::netbuf_type netbuf_type;
        // FIX: Need a much more cohesive way of doing this
        netbuf_type* netbuf = item.netbuf();
        delete netbuf;
#endif

        notify(typename event::receive_dequeued(item));

        datapump.dequeue_pop();
    }
}

// NOTE: All this demands inline-mode, but technically doesn't have to with a bit
// more work
template <class TDatapump, class TTransport, class TSubject, bool wrapped>
void DataPort<TDatapump, TTransport, TSubject, wrapped>::service()
{
    base_t::service();

    // FIX: For now, recv must happen in TTransport itself async/out-of-band
    // revise this so that we can indeed poll.  Disabled since 'addr'
    // requires initialization
/*
    {
        netbuf_type* nb;
        addr_t addr;

        // polled transport mode
        // FIX: Needs more work, really wants an 'available' call for polling
        // then a recv which we can do a std::move on
        // also , beware that 'recv' very likely will collide with any sockets
        // implementations
        if(transport.recv(&nb, &addr))
        {
            //subject.notify(0); // receive from transport event
            //datapump.transport_in(*nb, addr);
            //subject.notify(0); // queued into receive queue event
        }
    }
*/

    // anything queued for transport out?
    if(!base_t::datapump.transport_empty())
    {
        item_t& item = base_t::datapump.transport_front();
        netbuf_type& netbuf = *item.netbuf();
        const endpoint_type& addr = item.addr();

        notify(typename event::transport_sending(netbuf, addr));

        transport.send(netbuf, addr);

        notify(typename event::transport_sent(netbuf, addr));

        base_t::datapump.transport_pop();

        notify(typename event::send_dequeued(0, addr));
    }
}

template <class TDatapump, class TTransportDescriptor, class TSubject>
void DatapumpSubject<TDatapump, TTransportDescriptor, TSubject>::enqueue_for_send(
    netbuf_type&& nb,
    const endpoint_type& addr)
{
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
    const item_t& item = datapump.enqueue_out(std::move(nb), addr);
#else
    const item_t& item = datapump.enqueue_out(nb, addr);
    // FIX: probably needs additional housekeeping for non-inline flavor
#endif

    notify(typename event::send_queued(item));
}


template <class TDatapump, class TTransportDescriptor, class TSubject>
void DatapumpSubject<TDatapump, TTransportDescriptor, TSubject>::enqueue_from_receive(
    netbuf_type&& nb,
    const endpoint_type& addr)
{
    // think of datapump as a application-level queue, while
    // udp_data_recv sorta responds to a system-level queue
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
    const item_t& item = datapump.transport_in(std::move(nb), addr);
#else
    const item_t& item = datapump.transport_in(nb, addr);
    // FIX: probably needs additional housekeeping for non-inline flavor
#endif

    notify(typename event::receive_queued(item));
}

}
