#pragma once

namespace embr { namespace event {

template <class TNetBuf, class TAddr>
struct Base
{
    TNetBuf& netbuf;
    const TAddr& addr;

    Base(TNetBuf& netbuf, const TAddr& addr) :
        netbuf(netbuf),
        addr(addr)
    {}
};

template <class TItem>
struct ItemBase
{
    // item residing in a tx or rx queue
    TItem& item;

    ItemBase(TItem& item) : item(item) {}
};


// TODO: break out these dataport events into its own event_dataport.h
// event file.  Also do this for transport events


// appeared on transport
template <class TNetBuf, class TAddr>
struct TransportReceived : Base<TNetBuf, TAddr>
{
    TransportReceived(TNetBuf& netbuf, const TAddr& addr) :
        Base<TNetBuf, TAddr>(netbuf, addr)
    {}
};


// officially sent off over transport
template <class TNetBuf, class TAddr>
struct TransportSent : Base<TNetBuf, TAddr>
{
    TransportSent(TNetBuf& netbuf, const TAddr& addr) :
        Base<TNetBuf, TAddr>(netbuf, addr)
    {}
};

// queued for send out over transport
template <class TItem>
struct SendQueued : ItemBase<const TItem>
{
    SendQueued(const TItem& item) : ItemBase<const TItem>(item) {}
};

// about to send over transport
template <class TItem>
struct SendDequeuing : ItemBase<TItem>
{
    SendDequeuing(TItem& item) : ItemBase<TItem>(item) {}
};

// sent over transport, and just now removed from send queue
// NOTE: netbuf is not reliably available now.  use TransportSent
// if that is needed
template <class TAddr>
struct SendDequeued
{
    const TAddr& addr;

    // TODO: implement message_id tracking.  we can still match
    // up things to the old netbuf via a unique message_id
    template <class T>
    SendDequeued(T message_id, const TAddr& addr) : addr(addr) {}
};


// received from transport and now queued
template <class TItem>
struct ReceiveQueued : ItemBase<const TItem>
{
    ReceiveQueued(const TItem& item) : ItemBase<const TItem>(item) {}
};


// pulling out of receive-from-transport queue
template <class TItem>
struct ReceiveDequeuing : ItemBase<TItem>
{
    ReceiveDequeuing(TItem& item) : ItemBase<TItem>(item) {}
};


// completed processing of one item from receive-from-transport queue
// right after this, TNetBuf will be deallocated
template <class TItem>
struct ReceiveDequeued : ItemBase<TItem>
{
    ReceiveDequeued(TItem& item) : ItemBase<TItem>(item) {}
};


template <class TTransportDescriptor>
struct Transport
{
    typedef typename TTransportDescriptor::buffer_type netbuf_type;
    typedef typename TTransportDescriptor::endpoint_type addr_t;

    typedef Base<netbuf_type, addr_t> base;

    typedef TransportReceived<netbuf_type, addr_t>
        transport_received;
    typedef TransportSent<netbuf_type, addr_t>
        transport_sent;

    struct transport_sending : base
    {
        transport_sending(netbuf_type& nb, const addr_t& addr) : base(nb, addr) {}
    };
};

template <class TItem>
struct Datapump
{
    typedef TItem item_t;

    typedef ItemBase<item_t> item_base;

    typedef ReceiveDequeuing<item_t>
        receive_dequeuing;
    typedef ReceiveDequeued<item_t>
        receive_dequeued;
    typedef ReceiveQueued<item_t>
        receive_queued;
    typedef SendQueued<item_t>
        send_queued;
    typedef SendDequeued<typename item_t::addr_t>
        send_dequeued;
};


}}
