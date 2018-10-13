/**
 *
 * @file
 *
 * Revised datapump code, though still experimental.  Don't be surprised if we get a v3
 *
 */
#pragma once

#include <estd/forward_list.h>
#include <estd/chrono.h>
#include <estd/exp/memory_pool.h>

namespace embr { namespace experimental {

struct empty {};

template <class TRetryItem>
struct retry_item_traits;

// a reference implementation.  likely too generic to be of any use to anyone, but
// indicates what to do for a more specific scenario
template <class TPBuf, class TAddr>
struct BasicRetry
{
    typedef typename estd::remove_reference<TPBuf>::type pbuf_type;
    typedef TAddr addr_type;

    struct RetryItem
    {
        estd::chrono::steady_clock::time_point due;
        int counter = 0;

        // TODO: eventually do this via pbuf_traits
        bool is_confirmable(pbuf_type& pbuf) { return true; }

        bool is_acknowledge(pbuf_type& pbuf) { return true; }

        // evaluate whether the incoming item is an ACK matching 'this' item
        // expected to be a CON
        bool retry_match(RetryItem* compare_against)
        {
            return true;
        }
    };

    /// indicate that this item should be requeued
    /// TODO: disambiguate whether this includes regular queuing or not
    bool should_requeue(RetryItem* item, pbuf_type& pbuf)
    {
        return item->counter < 3;
    }

    /// indicate that this item should be dequeued (released and not retried again)
    bool should_dequeue(RetryItem* item, pbuf_type& pbuf)
    {
        return true;
    }

    /// @brief Indicate whether the specified retry item is ready for an actual resend
    /// Generally meaning has item's due date passed
    /// \param item
    /// \return
    bool ready_for_send(RetryItem* item)
    {
        return estd::chrono::steady_clock::now() >= item->due;
    }
};

// Would use transport descriptor, but:
// a) it's a little more unweildy than expected
// b) it's netbuf based, and this needs to be PBuf based
template <
        class TPBuf, class TAddr,
        class TRetryImpl = BasicRetry<TPBuf, TAddr>,
        class TItemBase = empty>
class Datapump2
{
public:
    typedef typename estd::remove_reference<TPBuf>::type pbuf_type;
    typedef TAddr addr_type;

private:
    TRetryImpl retry_impl;
    typedef typename estd::remove_reference<TRetryImpl>::type retry_impl_type;
    typedef typename retry_impl_type::RetryItem retry_item;

public:
    struct Item :
            TItemBase,
            retry_item
    {
        // TODO: eventually clean up and use all those forward_node helpers
        Item* _next;

        Item* next() { return _next; }
        void next(Item* n) { _next = n; }


        TPBuf pbuf;
        TAddr addr;

        /// @brief Reflects whether pbuf represents a confirmable message
        /// does not specifically indicate whether we are *still* interested in retry tracking though
        /// \return
        bool is_confirmable() { return retry_item::is_confirmable(pbuf); }

        /// @brief Reflects whether pbuf rpresents an ack message
        /// does not specifically indicate whether retry tracking cares about *this particular* ack though
        bool is_acknowledgement() { return true; }
    };

private:
    // TODO: Right now, memory_pool_ll is going to impose its own linked list onto item
    // but it would be nice to instead bring our own 'next()' calls and have memory_pool_ll
    // be able to pick those up.  This should amount to refining memory_pool_ll's usage of
    // node_traits
    estd::experimental::memory_pool_ll<Item, 10> pool;

    typedef estd::intrusive_forward_list<Item> list_type;

    list_type from_transport;
    list_type to_transport;
    list_type retry_list;

public:
    Item* allocate()
    {
        return pool.allocate();
    }

    void deallocate(Item* item)
    {
        pool.deallocate(item);
    }


    bool to_transport_ready() const
    {
        return !to_transport.empty();
    }

    /// @brief enqueue item into transport output queue
    void enqueue_to_transport(Item* item)
    {
        to_transport.push_front(*item);
    }

    void enqueue_to_transport(TPBuf pbuf, addr_type from_address)
    {
        Item* item = allocate();

        item->pbuf = pbuf;
        item->addr = from_address;

        enqueue_to_transport(item);
    }

    /// @brief dequeue item from transport output queue
    Item* dequeue_to_transport()
    {
        // FIX: should pull from 'back' (see dequeue_from_transport comments)
        Item& item = to_transport.front();
        to_transport.pop_front();
        return &item;
    }

    /// @brief enqueue item into transport receive-from queue
    void enqueue_from_transport(Item* item)
    {
        // always push_front here, we want to capture receive data as fast as possible
        from_transport.push_front(*item);
    }


    void enqueue_from_transport(TPBuf pbuf, addr_type from_address)
    {
        Item* item = allocate();

        item->pbuf = pbuf;
        item->addr = from_address;

        enqueue_from_transport(item);
    }

    bool from_transport_ready() const
    {
        return !from_transport.empty();
    }

    /// @brief dequeue item from transport receive-from queue
    Item* dequeue_from_transport()
    {
        // FIX: pull from 'back'.  Right now this is behaving as a LIFO buffer rather than FIFO
        Item& item = from_transport.front();
        from_transport.pop_front();
        return &item;
    }

    /// @brief adds to retry list
    ///
    /// including a linear search to splice it into proper time slot
    void add_to_retry(Item* sent_item)
    {
        // TODO: likely want to embed this into retry_impl
        // TODO: need to do requisite sort placement also
        typename list_type::iterator i = retry_list.begin();


        for(;i != retry_list.end(); i++)
        {
            Item& current = *i;
        }

        retry_list.push_front(*sent_item);
    }


    /// @brief inspects sent_item and decides whether we're still interested in it
    /// if so, add to retry list
    /// \param sent_item
    /// @return true if we added this to retry list
    bool evaluate_add_to_retry(Item* sent_item)
    {
        if(retry_impl.should_requeue(sent_item, sent_item->pbuf))
        {
            add_to_retry(sent_item);
            return true;
        }
        else
        {
            return false;
        }
    }

    /// @brief evaluates incoming item against any outstanding retries
    ///
    /// Specifically looking for ACKs to remove from retry list
    ///
    /// \param received_item buffer received over transport for inspection
    /// @returns Item* which was removed from retry list, or NULLPTR if none was found
    Item* evaluate_remove_from_retry(Item* received_item)
    {
        if(retry_impl.should_dequeue(received_item, received_item->pbuf))
        {
            // TODO: scour through retry list looking for *associated with* received_item
            // and remove that associated item - returning it here
            typename list_type::iterator i = retry_list.begin();

            for(;i != retry_list.end(); i++)
            {
                Item& current = *i;

                // evaluate if received_item ACK matches up to retry_list CON
                if(current.retry_match(received_item))
                {
                    // TODO: Need to remove the item still
                    // if so, remove the retry-tracked item from
                    // our retry list, then return the Item* so that
                    // others may operate on it (i.e. explicitly free it)
                    return &current;
                }
            }

            return NULLPTR;
        }
        else
            return NULLPTR;
    }

    /// @brief if the time is right, retrieve an Item* to send over transport as a retry
    ///
    /// if Item* indeed is ready, it is removed from the retry list and must be re-queued, if desired
    ///
    /// \return Item* or NULLPTR if nothing yet is ready
    Item* dequeue_retry_ready()
    {
        Item* item = &retry_list.front();
        if(item && retry_impl.ready_for_send(item))
        {
            retry_list.pop_front();
            return item;
        }
        else
            return NULLPTR;
    }
};

template <class TDatapump>
// simplified state-machine varient of megapowered Dataport, wired more specifically
// to retry logic than before
/// Notify mechanism surrounding a datapump
/// \tparam TDatapump
struct Dataport2
{
    TDatapump datapump;

    // NOTE: TDatapump and TTransport must have matching pbuf & addr types
    typedef typename TDatapump::pbuf_type pbuf_type;
    typedef typename TDatapump::addr_type addr_type;
    typedef typename TDatapump::Item datapump_item;

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

    struct NotifyContext
    {
        Dataport2* dataport;
        void* user;

        union
        {
            datapump_item* item;
            struct
            {
                pbuf_type pbuf;
                addr_type addr;
            } buf_addr;
        };
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


    void state(State s, datapump_item* item, void* user)
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
        if (datapump.from_transport_ready())
        {
            NotifyContext context { this, user };

            state(TransportInDequeuing, &context);
            datapump_item* item = datapump.dequeue_from_transport();
            state(TransportInDequeued, item, user);
            if (item->is_acknowledgement())
            {
                // FIX: Clean up this inconsistency where ACK processing has a special 'evaluating'
                // event sort of in lieu of item->should_retry()
                state(RetryEvaluating, item, user);
                datapump_item* removed = datapump.evaluate_remove_from_retry(item);
                if (removed != NULLPTR)
                    state(RetryDequeued, removed, user);
            }
        }
    }

    void process_to_transport(void* user = NULLPTR)
    {
        if(datapump.to_transport_ready())
        {
            NotifyContext context { this, user };

            state(TransportOutDequeuing, &context);
            datapump_item* item = datapump.dequeue_to_transport();
            state(TransportOutDequeued, item, user);
            // after we've definitely sent off the item, evaluate
            // whether it's a confirmable/retryable one
            if(item->is_confirmable())
            {
                state(RetryQueuing, item, user);
                if(datapump.evaluate_add_to_retry(item))
                    state(RetryQueued, item, user);
                else
                    // It's assumed that in the past, this retry item was queued up
                    // technically, RetryDequeued really means done attempting retries
                    state(RetryDequeued, item, user);
            }
        }
    }


    void send_to_transport(datapump_item* item, void* user = NULLPTR)
    {
        state(TransportOutQueuing, item, user);
        datapump.enqueue_to_transport(item);
        state(TransportOutQueued, item, user);
    }

    void process_retry(void* user = NULLPTR)
    {
        datapump_item* item_to_send = datapump.dequeue_retry_ready();

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
        datapump_item* item = datapump.allocate();
        send_to_transport(item);
    }

    /// @brief called when transport receives data, to queue up in our datapump/dataport
    ///
    /// this is mainly for async calls.  Queues into from_transport queue
    void receive_from_transport(pbuf_type pbuf, addr_type from_address, void* user = NULLPTR)
    {
        state(TransportInQueueing, pbuf, from_address, user);
        datapump.enqueue_from_transport(pbuf, from_address);
        state(TransportInQueued, pbuf, from_address, user);
    }
};

}}