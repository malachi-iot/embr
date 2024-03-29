/**
 * @file
 */
#pragma once

#include <estd/forward_list.h>
#include <estd/queue.h>
#include <estd/vector.h>

#ifdef FEATURE_CPP_MOVESEMANTIC
#include <estd/utility.h> // for std::forward

#ifdef FEATURE_CPP_VARIADIC
#ifndef ENABLE_EMBR_DATAPUMP_INLINE
#define ENABLE_EMBR_DATAPUMP_INLINE true
// NOTE: FEATURE_EMBR_DATAPUMP_INLINE is legacy behavior in process of being phased out
// we use alternate ENABLE_ paradigm so that we can have tri state:
// default (starts blank and becomes 1 or 0 depending on feature) , on (true), off (false)
#define FEATURE_EMBR_DATAPUMP_INLINE
#endif
#endif

#endif

namespace embr {

// FEATURE_EMBR_DATAPUMP_INLINE may be better suited to a policy than a compile
// type define.  Some 'netbuf' types could be a raw PBUF handle, in which case
// we might neither treat it as a pointer or a move-semantic object
#if ENABLE_EMBR_DATAPUMP_INLINE
#ifndef FEATURE_CPP_MOVESEMANTIC
#error Move semantic necessary for inline datapump
#endif
#ifndef FEATURE_CPP_VARIADIC
#error Variadic/emplacement necessary for inline datapump
#endif
#endif

template <size_t queue_depth = 10>
struct InlineQueuePolicy
{
    // Utilizing front/emplace helpers so that we can use aligned_storage more
    // transparently and only if necessary (some containers may not require it)
    template <class TItem>
    struct Queue
    {
        // guidance from https://en.cppreference.com/w/cpp/types/aligned_storage
        // so that we can actually have uninitialized chunks
        typedef TItem value_type;
#ifdef FEATURE_CPP_ALIGN
        typedef estd::queue<value_type,
            estd::layer1::deque<value_type, queue_depth,
            estd::experimental::aligned_storage_array_policy> > queue_type;
#else
        typedef estd::layer1::queue<value_type, queue_depth> queue_type;
#endif

        static value_type& front(queue_type& queue)
        {
            return queue.front();
        }

        template <class ...TArgs>
        static value_type& emplace(queue_type& queue, TArgs&&... args)
        {
            return queue.emplace(std::forward<TArgs&&>(args)...);
        }
    };
};

// NOTE: Temporarily leaving this in here since RETRY/RELIABLE code is temporarily
// disabled in need of a refactor
struct CoapAppDataPolicy
{
    // user/app data tracked in each item of datapump queue, in addition
    // to the necessities
    template <class TTransportDescriptor>
    struct AppData
    {
        typedef typename TTransportDescriptor::netbuf_type netbuf_type;
        typedef typename TTransportDescriptor::addr_t addr_t;

        // NOTE: Not yet used, and not bad but working on decoupling DataPump from coap altogether
        // so a different default policy would be good to supply this AppData
#ifdef FEATURE_MCCOAP_RELIABLE
        typename moducom::coap::experimental::Retry<netbuf_t, addr_t>::Metadata m_retry;
#endif
    };
};



struct EmptyAppDataInlineQueuePolicy :
        InlineQueuePolicy<>
{
    // TODO: Eventually use SFINAE instead of this
    template <class TTransportDescriptor>
    struct AppData {};
};


namespace experimental {

// opportunity to specialize because some netbufs have innate ref counters
// (like PBUF) while others we're gonna have to manage manually (through
// some clever use of shared_ptr, no doubt)
//
// this will also have implications on inline mode ... likely inline will
// become less used and non-inline will always be available
template <class TNetBuf>
struct DataPumpItem
{

};


}


template <class TTransportDescriptor, class TPolicy = EmptyAppDataInlineQueuePolicy >
/// @brief Contains input-from-transport and output-to-transport queues
/// \tparam TTransportDescriptor
/// \tparam TPolicy
class DataPump
{
public:
    typedef TTransportDescriptor transport_descriptor_t;
    typedef typename transport_descriptor_t::endpoint_type endpoint_type;
    typedef typename transport_descriptor_t::buffer_type buffer_type;
    typedef buffer_type netbuf_type;
#if ENABLE_EMBR_DATAPUMP_INLINE
    typedef netbuf_type pnetbuf_t;
#else
    typedef netbuf_type* pnetbuf_t;
#endif
    //typedef NetBufDecoder<netbuf_t&> decoder_t;
    typedef TPolicy policy_type;

public:
    // TODO: account for https://tools.ietf.org/html/rfc7252#section-4.2
    // though we have retry.h
    // To track that and other non-core-datagram behavior, stuff everything into AppData
    // via (experimentally) TPolicy.  Deriving from it so that it resolves to truly 0
    // bytes if no AppData is desired
    class Item :
            experimental::DataPumpItem<netbuf_type>,
            policy_type::template AppData<transport_descriptor_t>
    {
    public:
        typedef endpoint_type addr_t;

    private:
        pnetbuf_t m_netbuf;
        endpoint_type m_addr;

    public:
        Item() {}

#if ENABLE_EMBR_DATAPUMP_INLINE
        Item(netbuf_type&& netbuf, const endpoint_type& addr) :
            m_netbuf(std::move(netbuf)),
#else
        Item(netbuf_type& netbuf, const endpoint_type& addr) :
            m_netbuf(&netbuf),
#endif
            m_addr(addr)
        {}

        explicit Item(const Item& copy_from) :
            m_netbuf(copy_from.m_netbuf),
            m_addr(copy_from.m_addr)
        {

        }

#if ENABLE_EMBR_DATAPUMP_INLINE

        Item(Item&& move_from) :
            m_netbuf(std::move(move_from.m_netbuf)),
            m_addr(std::move(move_from.m_addr))
        {

        }

#endif

        // NOTE: more of an endpoint than an address
        const endpoint_type& addr() const { return m_addr; }

        netbuf_type* netbuf()
        {
#if ENABLE_EMBR_DATAPUMP_INLINE
            return &m_netbuf;
#else
            return m_netbuf;
#endif
        }

        const netbuf_type* netbuf() const
        {
#if ENABLE_EMBR_DATAPUMP_INLINE
            return &m_netbuf;
#else
            return m_netbuf;
#endif
        }

    };


private:
    typedef typename policy_type::template Queue<Item> queue_policy;
    typedef typename queue_policy::queue_type queue_type;

    queue_type incoming;
    queue_type outgoing;

public:
    // process data coming in from transport into coap queue
#if ENABLE_EMBR_DATAPUMP_INLINE
    const Item& transport_in(
            netbuf_type&& in,
#else
    const Item& transport_in(
            netbuf_type& in,
#endif
            const endpoint_type& addr);

    // ascertain whether any -> transport outgoing netbufs are present
    bool transport_empty() const
    {
        return outgoing.empty();
    }

    Item& transport_front()
    {
        return queue_policy::front(outgoing);
    }

    void transport_pop()
    {
        outgoing.pop();
    }

#if ENABLE_EMBR_DATAPUMP_INLINE
    // enqueue complete netbuf for outgoing transport to pick up
    const Item& enqueue_out(netbuf_type&& out, const endpoint_type& addr_out)
    {
        return queue_policy::emplace(outgoing, std::move(out), addr_out);
    }
#else
    // enqueue complete netbuf for outgoing transport to pick up
    const Item& enqueue_out(netbuf_type& out, const addr_t& addr_out)
    {
        outgoing.push(Item(out, addr_out));
        return outgoing.back();
    }
#endif

    // see if any netbufs were queued from transport in
    bool dequeue_empty() const { return incoming.empty(); }

    Item& dequeue_front() { return queue_policy::front(incoming); }

    // TODO: deprecated
    // dequeue complete netbuf which was queued from transport in
    netbuf_type* dequeue_in(endpoint_type* addr_in)
    {
        if(incoming.empty()) return NULLPTR;

        Item& f = queue_policy::front(incoming);
        netbuf_type* netbuf = f.netbuf();
        *addr_in = f.addr();

        return netbuf;
    }

    // call this after servicing input queued from transport in
    void dequeue_pop()
    {
        incoming.pop();
    }


#ifdef UNUSED
    // NOTE: Deprecated
    // inline-token, since decoder blasts over its own copy
    struct IncomingContext :
            moducom::coap::IncomingContext<addr_t, true>,
            DecoderContext<decoder_t>
    {
        friend class DataPump;

        typedef typename transport_descriptor_t::netbuf_t netbuf_t;
        typedef moducom::coap::IncomingContext<addr_t, true> base_t;

    private:
        DataPump& datapump;

        // decode header and token and begin option decode
        void prepopulate()
        {
            decoder_t& d = this->decoder();
            base_t::header(d.header());
            // This old code was for non-inline token, but because
            // I was erroneously using token2 (inline) then this call
            // to d.token worked
            //d.token(this->_token);
            this->token(d.token()); // overdoing/abusing it, but should get us there for now
            d.begin_option_experimental();
        }

    public:
        IncomingContext(DataPump& datapump, netbuf_t& netbuf) :
            DecoderContext<decoder_t>(netbuf),
            datapump(datapump)
        {}

        IncomingContext(DataPump& datapump, netbuf_t& netbuf, addr_t& addr) :
            DecoderContext<decoder_t>(netbuf),
            datapump(datapump)
        {
            this->addr = addr;
        }

        // marks end of input processing
        void deallocate_input()
        {
#ifndef FEATURE_EMBR_DATAPUMP_INLINE
            // FIX: Need a much more cohesive way of doing this
            delete &this->decoder().netbuf();
#endif
            datapump.dequeue_pop();
        }

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
        void respond(NetBufEncoder<netbuf_t>& encoder)
        {
            // lwip netbuf has a kind of 'shrink to fit' behavior which is best applied
            // sooner than later - the complete is perfectly suited to that.
            encoder.complete();
            datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), base_t::address());
        }
#else
        void respond(const NetBufEncoder<netbuf_t&>& encoder)
        {
            encoder.complete();
            datapump.enqueue_out(encoder.netbuf(), base_t::address());
        }
#endif
    };

    //!
    //! \brief service
    //! \param prepopulate_context gathers header, token and initiates option processing
    //!
    void service(void (*f)(IncomingContext&), bool prepopulate_context)
    {
        if(!dequeue_empty())
        {
            // TODO: optimize this so that we can avoid copying addr around
            addr_t addr;

            IncomingContext context(*this, *dequeue_in(&addr));

            context.addr = addr;

            if(prepopulate_context) context.prepopulate();

            f(context);

        }
    }

    //!
    //! \brief service
    //! \param prepopulate_context gathers header, token and initiates option processing
    //!
    template <class TObservableCollection>
    void service(void (*f)(IncomingContext&, ObservableRegistrar<TObservableCollection>&),
                 ObservableRegistrar<TObservableCollection>& observable_registrar,
                 bool prepopulate_context)
    {
        if(!dequeue_empty())
        {
            // TODO: optimize this so that we can avoid copying addr around
            addr_t addr;

            IncomingContext context(*this, *dequeue_in(&addr));

            context.addr = addr;

            if(prepopulate_context) context.prepopulate();

            f(context, observable_registrar);

        }
    }
#endif
};

}
