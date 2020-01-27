/**
 * @file
 */
#pragma once

#include "datapump-v2.h"

namespace embr { namespace experimental {


template <class TDatapumpWithRetry>
struct DataportFnPtrImpl
{

};

namespace event {

template <class TPBuf, class TAddr>
struct DataportBase
{
    typedef TPBuf pbuf_type;
    typedef TAddr addr_type;

    pbuf_type pbuf;
    addr_type addr;
};

}

template <class TDatapumpWithRetry>
// simplified state-machine varient of megapowered Dataport, wired more specifically
// to retry logic than before
/// Notify mechanism surrounding a datapump
/// \tparam TDatapumpWithRetry
struct Dataport2 : DataportFnPtrImpl<TDatapumpWithRetry>
{
    typedef DataportFnPtrImpl<TDatapumpWithRetry> impl_type;
    typedef TDatapumpWithRetry datapump_type;
    typedef TDatapumpWithRetry retry_type;

    TDatapumpWithRetry _datapump;

    datapump_type& datapump() { return _datapump; }
    retry_type& retry() { return _datapump; }

    // NOTE: TDatapumpWithRetry and TTransport must have matching pbuf & addr types
    typedef typename datapump_type::pbuf_type pbuf_type;
    typedef typename datapump_type::addr_type addr_type;
    typedef typename datapump_type::item_type item_type;

    enum State
    {
        TransportInReceiving,
        TransportInReceived,
        TransportInQueueing,
        TransportInQueued,
        TransportInDequeuing,
        // application code is expected to listen to this notification
                TransportInDequeued,
        TransportOutSending,
        TransportOutSent,
        TransportOutQueuing,
        TransportOutQueued,
        TransportOutDequeuing,
        // transport code is expected to listen to this notification
        // and respond by sending
                TransportOutDequeued,
        RetryQueuing,
        RetryQueued,
        /// evaluating whether item should be permanantly removed from retry list
        /// used right now specifically when about to process an ACK
        /// TODO: Seems like we could combine this with 'temporary' removal from list while it's living in to_transport queue
                RetryEvaluating,    ///< TEST
        /// Indicates we no longer will attempt retries for this item
                RetryDequeued,
    };

    // NOTE: Looking like we might not even need an instance field
    // for state here
    State _state;

    struct NotifyEvent
    {
        union
        {
            item_type* item;
            event::DataportBase<pbuf_type, addr_type> buf_addr;
            struct
            {
                pbuf_type pbuf;
                addr_type addr;
            } buf_addr_old;
        };

        /// \brief helper method just in case you aren't 100% sure how to get pbuf out of context
        /// \param state
        /// \return
        pbuf_type& pbuf(State state)
        {
            switch(state)
            {
                default: return item->pbuf;
            }
        }

        addr_type& addr(State state)
        {
            switch(state)
            {
                default: return item->addr;
            }
        }

        NotifyEvent() {}

        NotifyEvent(item_type* item) : item(item) {}
    };

    struct NotifyContext : NotifyEvent
    {
        Dataport2* dataport;
        void* user;

        NotifyContext(Dataport2* dataport, void* user) :
            dataport(dataport),
            user(user)
        {}

        NotifyContext(Dataport2* dataport, void* user, item_type* item) :
            NotifyEvent(item),
            dataport(dataport),
            user(user)
        {}
    };

    typedef void (*notify_fn)(State state, NotifyContext* context);

    /// callback
    notify_fn notifier;

    void notify(NotifyContext* context)
    {
        if(notifier != NULLPTR) notifier(_state, context);
    }

    // state also brings along notification, so pass in notification
    // context when applicable
    void state(State s, NotifyContext* context = NULLPTR)
    {
        if(s != _state)
        {
            _state = s;
            notify(context);
        }
    }


    void state(State s, item_type* item, void* user)
    {
        NotifyContext context{ this, user, item };
        state(s, &context);
    }

    void state(State s, pbuf_type pbuf, addr_type addr, void* user)
    {
        NotifyContext context{ this, user };

        context.buf_addr.pbuf = pbuf;
        context.buf_addr.addr = addr;

        state(s, &context);
    }

    State state() const { return _state; }

    void process_from_transport(void* user = NULLPTR)
    {
        if (datapump().from_transport_ready())
        {
            NotifyContext context { this, user };

            state(TransportInDequeuing, &context);
            item_type* item = datapump().dequeue_from_transport();
            state(TransportInDequeued, item, user);
            if (item->is_acknowledge())
            {
                state(RetryEvaluating, item, user);
                item_type* removed = retry().evaluate_remove_from_retry(item);
                if (removed != NULLPTR)
                    state(RetryDequeued, removed, user);
            }
        }
    }

    void process_to_transport(void* user = NULLPTR)
    {
        if(datapump().to_transport_ready())
        {
            NotifyContext context { this, user };

            state(TransportOutDequeuing, &context);
            item_type* item = datapump().dequeue_to_transport();
            state(TransportOutDequeued, item, user);
            // after we've definitely sent off the item, evaluate
            // whether it's a confirmable/retryable one
            if(item->is_confirmable())
            {
                state(RetryQueuing, item, user);
                if(retry().evaluate_add_to_retry(item))
                    state(RetryQueued, item, user);
                else
                    // It's assumed that in the past, this retry item was queued up
                    // technically, RetryDequeued really means done attempting retries
                    state(RetryDequeued, item, user);
            }
        }
    }


    void send_to_transport(item_type* item, void* user = NULLPTR)
    {
        state(TransportOutQueuing, item, user);
        datapump().enqueue_to_transport(item);
        state(TransportOutQueued, item, user);
    }

    void process_retry(void* user = NULLPTR)
    {
        item_type* item_to_send = retry().dequeue_retry_ready();

        if(item_to_send != NULLPTR)
            // process_to_transport will handle requeuing portion
            send_to_transport(item_to_send);
    }


    void process(void* user = NULLPTR)
    {
        process_from_transport(user);
        process_retry(user);
        process_to_transport(user);
    }


    /// @brief Queue up for send to transport
    /// \param pbuf
    /// \param to_address
    void send_to_transport(pbuf_type& pbuf, addr_type to_address, void* user = NULLPTR)
    {
        // FIX: resolve descrepency between formats of TransportOutQueuing
        // by allocating an Item *here* instead of in the datapump helper function
        item_type* item = datapump().allocate();
        send_to_transport(item);
    }

    /// @brief called when transport receives data, to queue up in our datapump/dataport
    ///
    /// this is mainly for async calls.  Queues into from_transport queue
    void received_from_transport(pbuf_type pbuf, addr_type from_address, void* user = NULLPTR)
    {
        state(TransportInQueueing, pbuf, from_address, user);
        item_type* item = datapump().enqueue_from_transport(pbuf, from_address);
        state(TransportInQueued, item, user);
    }
};


}}