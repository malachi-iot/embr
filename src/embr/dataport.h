#pragma once

#include <estd/type_traits.h>
#include "events.h"
#include "datapump.h"

namespace embr {

// DataPort events, specifically
template <class TDatapump>
struct DataPortEvents :
    // Be careful, TDatapumpWithRetry is being used as transport description.  In this case, it's OK
    event::Transport<TDatapump>,
    event::Datapump<typename TDatapump::Item> {};

// NOTE: For now, this actually is just a test event to make sure our event system
// is working as expected
struct dataport_initialized_event {};


// FIX: needs better name
// right now this actually represents a virtual interface specifically to datapump
template <class TTransportDescription>
struct DataPortVirtual
{
    typedef TTransportDescription transport_descriptor_t;
    typedef typename TTransportDescription::netbuf_type netbuf_type;
    typedef typename TTransportDescription::endpoint_type endpoint_type;

    // for now, not overpromising since the virtual version if this would be broken
    // (would only service datapump, not dataport)
    //virtual void service() = 0;

    virtual void enqueue_for_send(netbuf_type&& nb, const endpoint_type& addr) = 0;
    virtual void enqueue_from_receive(netbuf_type&& nb, const endpoint_type& addr) = 0;
};


// to virtualize the wrapper
// we don't virtual by default because dataport extends its functionality through
// composition (using alternate TDatapumpWithRetry, TTransport, etc) so as to resolve at
// compile time.
// the situations that virtualized calls are needed are fewer, but present.  That's
// what this wrapper class is for
template <class TDataPort>
struct DataPortWrapper :
    DataPortVirtual<typename std::remove_reference<TDataPort>::type::transport_descriptor_t>
{
    TDataPort dataport;

    typedef typename std::remove_reference<TDataPort>::type dataport_t;

    //typedef typename dataport_t::datapump_t transport_description_t;
    typedef typename dataport_t::netbuf_type netbuf_type;
    typedef typename dataport_t::endpoint_type endpoint_type;

    DataPortWrapper(dataport_t& dataport) :
        dataport(dataport) {}

    //virtual void service() override { dataport.service(); }

    virtual void enqueue_for_send(netbuf_type&& nb, const endpoint_type& addr) override
    {
        dataport.enqueue_for_send(std::move(nb), addr);
    }

    virtual void enqueue_from_receive(netbuf_type&& nb, const endpoint_type& addr) override
    {
        dataport.enqueue_from_receive(std::move(nb), addr);
    }
};


// Datapump API surface needs to be shared all over an app,
// Subject needs to be tightly bound to DataPump for Observer-Subject to work
// Transport interaction need not be shared all over, but at a low technical
// level, due to metadata behavior, at least *does* need potentially to be shared
// ala a hidden/later inherited field
// So we break up DataPort so that hopefully we can use DataPumpSubject and shed
// app-layer dependencies on transport for good
template <class TDatapump, class TTransportDescriptor, class TSubject>
struct DatapumpSubject
{
    typedef typename std::remove_reference<TDatapump>::type datapump_t;
    typedef typename datapump_t::Item item_t;
    typedef typename datapump_t::netbuf_type netbuf_type;
    typedef typename datapump_t::endpoint_type endpoint_type;
    // we do, however, need specifics about our netbuf and addr structure
    // since datapump uses those.  Right now datapump 'supplies its own' but
    // eventually want to hang that off transport descriptor
    typedef TTransportDescriptor transport_descriptor_t;
    typedef DataPortEvents<TDatapump> event;

    TDatapump datapump;
    TSubject subject;

    DatapumpSubject(TSubject& subject) :
        subject(subject) {}

    // services just from-transport queue.  does not directly interact
    // with transport itself
    void service();

    // FIX: Still need a graceful solution for wrapped/unwrapped
    template <class TEvent>
    void notify(const TEvent& e, bool wrapped = true)
    {
        if(wrapped)
        {
            // FIX: This is not gonna work for polymorph-izing service,
            // so tend to that when it's time (right now service() call
            // isn't used virtually from anyone)
            DataPortWrapper<DatapumpSubject&> wrapped(*this);

            subject.notify(e, wrapped);
        }
        else
        {
            subject.notify(e, *this);
        }
    }


    // application send out -> datapump -> (eventual transport send)
    void enqueue_for_send(netbuf_type&& nb, const endpoint_type& addr);
    // transport receive in -> datapump -> (eventual application process)
    void enqueue_from_receive(netbuf_type&& nb, const endpoint_type& addr);
};

// DataPump and Transport combined, plus a subject to send out
// notifications via
// wrapped = whether to wrap up 'this' in DataPortWrapper in order to virtualize
// it on the way out.  In the future, perhaps merely extend directly from DataPortWrapper
// when the flag is set
template <class TDatapump, class TTransport, class TSubject, bool wrapped = true>
struct DataPort : DatapumpSubject<
        TDatapump,
        typename TTransport::transport_descriptor_t,
        TSubject>
{
    typedef TTransport transport_t;
    typedef typename TTransport::transport_descriptor_t transport_descriptor_t;

    typedef DatapumpSubject<TDatapump, transport_descriptor_t, TSubject> base_t;

    typedef DataPortEvents<TDatapump> event;
    typedef typename base_t::item_t item_t;
    typedef typename base_t::netbuf_type netbuf_type;
    typedef typename base_t::endpoint_type endpoint_type;

    // Embedded dataport-transport helper required at this time.
    // Eventually perhaps consider making this reference-friendly
    TTransport transport;

    template <class TEvent>
    void notify(const TEvent& e)
    {
        base_t::notify(e, wrapped);
    }

public:
/*
    DataPort() : transport(this)
    {
        notify(dataport_initialized_event {});
    } */

    template <class ...TArgs>
    DataPort(TSubject& subject, TArgs&&...args) :
        base_t(subject),
        // NOTE: Transport init *must* take dataport pointer as 1st parameter
        transport(this, std::forward<TArgs&&>(args)...)
    {
        notify(dataport_initialized_event {});
    }

    // FIX: eventually evaluates transport in
    // currently evaluates to-transport queue and spits out to transport if present
    // also evalutes from-transport queue, though presently counts on external party
    // to populate that
    void service();
};


/// Makes a dataport using default policies for datapump
template <class TTransport, class TSubject, class ...TArgs>
DataPort<
    embr::DataPump<
        typename TTransport::transport_descriptor_t
        >,
    TTransport,
    TSubject> 
make_dataport(TSubject& s, TArgs&&...args)
{
    return DataPort<
                embr::DataPump<
                    typename TTransport::transport_descriptor_t
                    >,
                TTransport,
                TSubject>
            (s, std::forward<TArgs&&>(args)...);
}

// TODO: Make a make_dataport which accepts an explicit DataPump policy
// (perhaps through tagging)

}
