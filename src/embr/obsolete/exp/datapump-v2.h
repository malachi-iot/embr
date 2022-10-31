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

#include "datapump-core-v2.h"
#include "pbuf.h"
#include "retry-v2.h"

namespace embr { namespace experimental {

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



}}