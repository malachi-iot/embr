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

// Would use transport descriptor, but:
// a) it's a little more unweildy than expected
// b) it's netbuf based, and this needs to be PBuf based
template <
        class TPBuf, class TAddr,
        class TRetryImpl = empty,
        class TItemBase = empty>
class Datapump2
{
    typedef typename estd::remove_reference<TPBuf>::type pbuf_type;
    typedef TAddr addr_type;

    enum State
    {
        TransportInQueueing,
        TransportInQueued,
        TransportInDequeuing,
        TransportInDequeued,
        TransportOutQueuing,
        TransportOutQueued,
        TransportOutDequeuing,
        TransportOutDequeued
    };

    // TODO: Make this part of TRetryImpl
    struct RetryItem
    {
        estd::chrono::steady_clock::time_point due;

        // TODO: eventually do this via pbuf_traits
        static bool is_confirmable(pbuf_type& pbuf) { return true; }
    };

    struct Item :
            TItemBase,
            RetryItem
    {
        // TODO: eventually clean up and use all those forward_node helpers
        Item* _next;

        Item* next() { return _next; }
        void next(Item* n) { _next = n; }


        TPBuf pbuf;
        TAddr addr;
    };

    typedef void (*notify_fn)(State state);

    // TODO: Right now, memory_pool_ll is going to impose its own linked list onto item
    // but it would be nice to instead bring our own 'next()' calls and have memory_pool_ll
    // be able to pick those up.  This should amount to refining memory_pool_ll's usage of
    // node_traits
    estd::experimental::memory_pool_ll<Item, 10> pool;

    estd::intrusive_forward_list<Item> from_transport;
    estd::intrusive_forward_list<Item> to_transport;
    estd::intrusive_forward_list<Item> retry_list;

public:
    Item* allocate()
    {
        return pool.allocate();
    }

    void deallocate(Item* item)
    {
        pool.deallocate(item);
    }

    /// @brief enqueue item into transport output queue
    void enqueue_to_transport(Item* item)
    {
    }

    /// @brief dequeue item from transport output queue
    Item* dequeue_to_transport()
    {

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
    void add_to_retry(Item* item)
    {

    }
};

}}