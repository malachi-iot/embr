/**
 * @file
 */
#pragma once

#include "datapump-core-v2.h"

namespace embr { namespace experimental {

template <class TRetryItem>
struct retry_item_traits;

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


}}