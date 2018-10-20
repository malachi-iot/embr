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
#include <estd/optional.h>
#include <estd/exp/memory_pool.h>

#include "pbuf.h"

namespace embr { namespace experimental {

struct empty {};

template <class TRetryItem>
struct retry_item_traits;

// datapump-specific portion of its item.  specified out here mainly so
// retry logic can pass it around a bit more easily
template <class TPBuf, class TAddr>
struct Datapump2CoreItem
{
    // TODO: eventually clean up and use all those forward_node helpers
    Datapump2CoreItem* _next;

    // node_traits will automatically forward cast for us.  Generally accepted
    // practice, but be careful!
    void* next() { return _next; }
    void next(Datapump2CoreItem* n) { _next = n; }

    // major difference between PBUF and netbuf is PBUF state is not expected to change
    // during internal datapump operations, whereas netbuf have positioning data
    TPBuf pbuf;
    TAddr addr;

    typedef typename estd::remove_reference<TPBuf>::type pbuf_type;
    typedef TAddr addr_type;
};

// a reference implementation.  likely too generic to be of any use to anyone, but
// indicates what to do for a more specific scenario
template <class TPBuf, class TAddr, class TClock = estd::chrono::steady_clock>
struct BasicRetry
{
    typedef typename estd::remove_reference<TPBuf>::type pbuf_type;
    typedef TAddr addr_type;
    typedef TClock clock_type;
    typedef embr::experimental::pbuf_traits<pbuf_type> pbuf_traits;

    // NOTE: playing some games explicitly stating this so that retry code doesn't
    // have to couple quite so tightly with datapump code - mainly to aid in compile-time
    // convenience of type checking
    typedef Datapump2CoreItem<TPBuf, TAddr> datapump_item;

    // NOTE: Make sure we don't HAVE to use RetryItemBase - specifically,
    // we'll probably want more fine grained counter control
    template <class TCounter = int, class TBase = datapump_item>
    struct RetryItemBase : TBase
    {
#ifdef UNIT_TESTING
    public:
#else
    private:
#endif
        typename TClock::time_point _due;

        /// Indicates number of retry/queue attempts made on this item so far
        TCounter _counter = 0;

    public:
        TCounter counter() const { return _counter; }

        // putting this into accessor pattern so that optimized flavors of due
        // tracking integrate more easily
        typename TClock::time_point due() const { return _due; }

        /// @brief called when this item is actually added to retry list
        void queued()
        {
            // TODO: this is where we'll assign due as well
            _counter++;
        }

        // TODO: probably replace this with a specialized operator <
        bool less_than(const RetryItemBase& compare_to)
        {
            return _due < compare_to.due();
        }
    };

    // NOTE: Not all platforms have a steady clock.  This might cause compilation issues
    // even on code bases that don't use RetryItemBase
    struct RetryItem : RetryItemBase<>
    {
        typedef RetryItemBase<> base_type;

        bool is_confirmable() { return true; }

        bool is_acknowledge() { return true; }

        // evaluate whether the incoming item is an ACK matching 'this' item
        // expected to be a CON.  Comparing against something without retry metadata
        // because a JUST RECEIVED ACK item won't have any retry metadata yet
        bool retry_match(RetryItem* compare_against)
        {
            return true;
        }
    };

    /// @brief indicate that this item should be a part of retry list
    /// It's expected we've already filtered that this IS a CON/retry-wanting
    /// message by the time we get here
    bool should_queue(RetryItem* item)
    {
        return item->counter() < 3;
    }

    /// @brief Indicate whether the specified retry item is ready for an actual resend
    /// Generally meaning has item's due date passed
    /// \param item
    /// \return
    bool ready_for_send(RetryItem* item)
    {
        return clock_type::now() >= item->due();
    }
};

template <
        class TPBuf, class TAddr,
        class TRetryImpl = BasicRetry<TPBuf, TAddr>,
        class TItem = typename TRetryImpl::RetryItem >
class Retry2
{
    TRetryImpl retry_impl;

public:
    typedef typename estd::remove_reference<TRetryImpl>::type retry_impl_type;
    //typedef typename retry_impl_type::RetryItem retry_item;
    typedef TItem item_type;
    typedef item_type* pointer;

#ifdef UNIT_TESTING
public:
#else
protected:
#endif
    typedef estd::intrusive_forward_list<item_type> list_type;
    list_type retry_list;
    typedef typename list_type::iterator iterator;

    // TODO: will very likely want preceding item also to do a more efficient delete
    // internal call, removes the item *after* the specified preceding_item from retry list
    void _remove_from_retry(estd::optional<iterator> preceding_item)
    {
        if(preceding_item)
            retry_list.erase_after(*preceding_item);
        else
            // if there is no preceding item, it is assumed we're pulling off the front item
            // another artifact of having no 'before begin'
            retry_list.pop_front();
    }

    // All this 'before begin' compensation is a little annoying, but at least we aren't
    // polluting iterators with that extra information 100% of the time.
    void _add_to_retry(estd::optional<iterator> preceding, pointer item)
    {
        if(preceding)
            retry_list.insert_after(*preceding, *item);
        else
            retry_list.push_front(*item);
    }

public:
    ///
    /// @brief inspects sent_item and decides whether we're interested in adding it
    /// to the retry queue.
    /// It is expected some simple filtering happens before we get here (i.e. non reliable
    /// messages are not evaluated for placement in the retry list)
    /// \param sent_item
    /// @return true if we added this to retry list
    bool evaluate_add_to_retry(pointer sent_item)
    {
        if(retry_impl.should_queue(sent_item))
        {
            add_to_retry(sent_item);
            sent_item->queued();
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
    pointer evaluate_remove_from_retry(pointer received_item)
    {
        //if(retry_impl.should_dequeue(received_item, received_item->pbuf))
        {
            // scour through retry list looking for *associated with* received_item
            // and remove that associated item - returning it here
            typename list_type::iterator i = retry_list.begin();
            // TODO: Not optimal/optimized, but convenient
            typename estd::optional<typename list_type::iterator> previous;

            for(;i != retry_list.end(); i++)
            {
                item_type& current = *i;

                // evaluate if received_item ACK matches up to retry_list CON
                if(current.retry_match(received_item))
                {
                    // if so, remove the retry-tracked item from
                    // our retry list, then return the Item* so that
                    // others may operate on it (i.e. explicitly free it)
                    _remove_from_retry(previous);

                    return &current;
                }

                previous = i;
            }

            return NULLPTR;
        }
    }

    /// @brief if the time is right, retrieve an Item* to send over transport as a retry
    ///
    /// if Item* indeed is ready, it is removed from the retry list and must be re-queued, if desired
    ///
    /// \return Item* or NULLPTR if nothing yet is ready
    pointer dequeue_retry_ready()
    {
        pointer item = &retry_list.front();
        if(item && retry_impl.ready_for_send(item))
        {
            retry_list.pop_front();
            return item;
        }
        else
            return NULLPTR;
    }

    /// @brief adds to retry list
    ///
    /// including a linear search to splice it into proper time slot
    void add_to_retry(pointer sent_item)
    {
        // TODO: likely want to embed this into retry_impl
        // TODO: need to do requisite sort placement also
        typename list_type::iterator i = retry_list.begin();
        estd::optional<iterator> previous;


        for(;i != retry_list.end(); i++)
        {
            item_type& current = *i;

            // Look to insert sent_item in time slot just before item to be sent after it
            // or in other words, insert just before first encounter of current.due >= sent_item.due,
            if(!sent_item->less_than(current))
            {
                _add_to_retry(previous, sent_item);
                return;
            }

            previous = i;
        }

        // If loop completed, that means every present item.due was < sent_item.due
        // OR no items present at all.  In either case, we want to 'add to the end'
        _add_to_retry(previous, sent_item);
    }
};


// use this flag to indicate to add FIFO packets to the 'back' of
// the container, and retrieve from the 'front'.  This is the efficient
// wait to interact with 'forward_list_with_back'
#define FEATURE_EMBR_DATAPUMP_ENQUEUE_BACK

// Would use transport descriptor, but:
// a) it's a little more unweildy than expected
// b) it's netbuf based, and this needs to be PBuf based
template <
        class TPBuf, class TAddr,
        class TItem = Datapump2CoreItem<TPBuf, TAddr> >
class Datapump2
{
public:
    typedef typename estd::remove_reference<TPBuf>::type pbuf_type;
    typedef TAddr addr_type;

private:
public:
    // TODO: Do static asserts to make sure we have at least conformance to Datapool2CoreItem
    typedef TItem item_type;
    typedef item_type* pointer;

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    // TODO: Right now, memory_pool_ll is going to impose its own linked list onto item
    // but it would be nice to instead bring our own 'next()' calls and have memory_pool_ll
    // be able to pick those up.  This should amount to refining memory_pool_ll's usage of
    // node_traits
    estd::experimental::memory_pool_ll<item_type, 10> pool;

    typedef estd::intrusive_forward_list_with_back<item_type> list_type;

    list_type from_transport;
    list_type to_transport;

    typedef typename list_type::iterator iterator;

public:
    pointer allocate()
    {
        return pool.allocate();
    }

    void deallocate(pointer item)
    {
        pool.deallocate(item);
    }


    bool to_transport_ready() const
    {
        return !to_transport.empty();
    }

    /// @brief enqueue item into transport output queue
    void enqueue_to_transport(pointer item)
    {
#ifdef FEATURE_EMBR_DATAPUMP_ENQUEUE_BACK
        to_transport.push_back(*item);
#else
        to_transport.push_front(*item);
#endif
    }

    void enqueue_to_transport(TPBuf pbuf, addr_type from_address)
    {
        pointer item = allocate();

        item->pbuf = pbuf;
        item->addr = from_address;

        enqueue_to_transport(item);
    }

    /// @brief dequeue item from transport output queue
    pointer dequeue_to_transport()
    {
#ifdef FEATURE_EMBR_DATAPUMP_ENQUEUE_BACK
        // enqueue to back, dequeue from front
        item_type& item = to_transport.front();
        to_transport.pop_front();
        return &item;
#else
        // FIX: should pull from 'back' (see dequeue_from_transport comments)
#endif
    }

    /// @brief enqueue item into transport receive-from queue
    void enqueue_from_transport(pointer item)
    {
#ifdef FEATURE_EMBR_DATAPUMP_ENQUEUE_BACK
        from_transport.push_back(*item);
#else
        // always push_front here, we want to capture receive data as fast as possible
        from_transport.push_front(*item);
#endif
    }


    pointer enqueue_from_transport(TPBuf pbuf, addr_type from_address)
    {
        pointer item = allocate();

        item->pbuf = pbuf;
        item->addr = from_address;

        enqueue_from_transport(item);

        return item;
    }

    bool from_transport_ready() const
    {
        return !from_transport.empty();
    }

    /// @brief dequeue item from transport receive-from queue
    pointer dequeue_from_transport()
    {
#ifdef FEATURE_EMBR_DATAPUMP_ENQUEUE_BACK
        item_type& item = from_transport.front();
        from_transport.pop_front();
        return &item;
#else
        // FIX: pull from 'back'.  Right now this is behaving as a LIFO buffer rather than FIFO
        // to facilitate that, work on a forward_list_with_tail or similar
#error Not supported at this time.  Will need a doubly linked list, or reverse_list_with_back
#endif
    }

};


template <
        class TPBuf, class TAddr,
        class TRetryImpl = BasicRetry<TPBuf, TAddr>,
        class TItem = typename TRetryImpl::RetryItem >
class DatapumpWithRetry2 :
        public Datapump2<TPBuf, TAddr, TItem>,
        public Retry2<TPBuf, TAddr, TRetryImpl, TItem>
{
public:
    typedef TItem item_type;
};

template <class TDatapumpWithRetry>
// simplified state-machine varient of megapowered Dataport, wired more specifically
// to retry logic than before
/// Notify mechanism surrounding a datapump
/// \tparam TDatapumpWithRetry
struct Dataport2
{
    TDatapumpWithRetry datapump;

    // NOTE: TDatapumpWithRetry and TTransport must have matching pbuf & addr types
    typedef typename TDatapumpWithRetry::pbuf_type pbuf_type;
    typedef typename TDatapumpWithRetry::addr_type addr_type;
    typedef typename TDatapumpWithRetry::item_type item_type;

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
            item_type* item;
            struct
            {
                pbuf_type pbuf;
                addr_type addr;
            } buf_addr;
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
        if (datapump.from_transport_ready())
        {
            NotifyContext context { this, user };

            state(TransportInDequeuing, &context);
            item_type* item = datapump.dequeue_from_transport();
            state(TransportInDequeued, item, user);
            if (item->is_acknowledge())
            {
                state(RetryEvaluating, item, user);
                item_type* removed = datapump.evaluate_remove_from_retry(item);
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
            item_type* item = datapump.dequeue_to_transport();
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


    void send_to_transport(item_type* item, void* user = NULLPTR)
    {
        state(TransportOutQueuing, item, user);
        datapump.enqueue_to_transport(item);
        state(TransportOutQueued, item, user);
    }

    void process_retry(void* user = NULLPTR)
    {
        item_type* item_to_send = datapump.dequeue_retry_ready();

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
        item_type* item = datapump.allocate();
        send_to_transport(item);
    }

    /// @brief called when transport receives data, to queue up in our datapump/dataport
    ///
    /// this is mainly for async calls.  Queues into from_transport queue
    void received_from_transport(pbuf_type pbuf, addr_type from_address, void* user = NULLPTR)
    {
        state(TransportInQueueing, pbuf, from_address, user);
        item_type* item = datapump.enqueue_from_transport(pbuf, from_address);
        state(TransportInQueued, item, user);
    }
};

}}