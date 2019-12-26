/**
 * @file
 *  Experimental 
 *  Informally a retry-v3
 *  FreeRTOS-specific retry queue implementation
 *  Presumes datagram-style behavior:
 *  1) sessionless, no inbuilt sequence mechanism
 *  2) non blocking/async
 * 
 *  Also depends heavily on estd::streambuf, and depends it having
 *  a netbuf-style 'reset' capability (perhaps done through pubseekoff)
 */
#pragma once

#include <estd/internal/platform.h>
#include <estd/forward_list.h>
#include <estd/algorithm.h>

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#else
#if defined(UNIT_TESTING) and !defined(ESTD_FREERTOS)
// do nothing here, for GNU testing of this FreeRTOS specific area
#else
#include <timers.h>
#endif
#endif

namespace embr { namespace experimental {

// We expect TTransport to be conforming to something like ../lwip/transport.h
//    an important component of that is that it accepts a TStreambuf for input
// We expect TRetryPolicyImpl to help us choose how long retry delays are as well
//    as how many times to retry, and how to compare our streambuf to incoming
//    streambuf to make sure it's an app-specific match (e.g. matching on CoAP MID)
template <class TTransport, class TRetryPolicyImpl>
struct RetryManager
{
    TRetryPolicyImpl policy_impl;

    typedef typename TRetryPolicyImpl::key_type key_type;
    typedef typename TTransport::endpoint_type endpoint_type;
    typedef typename TTransport::ostreambuf_type ostreambuf_type;
    typedef typename TTransport::istreambuf_type istreambuf_type;
    typedef typename TRetryPolicyImpl::item_policy_impl_type item_policy_impl_type;

    key_type extract_key(istreambuf_type& streambuf)
    {
        return policy_impl.extract_key(streambuf);
    }


    struct QueuedItem : 
        estd::experimental::forward_node_base_base<QueuedItem*>,
        item_policy_impl_type
    {
        // endpoint that we want to send to.  For ipv4 this is IP address and port
        endpoint_type endpoint;
        // streambuf to (repeatedly) send to aforementioned endpoint
        ostreambuf_type& streambuf;
        unsigned retry_count;
        // NOTE: may or may not want to cache this here, but probably yes
        key_type key;

        QueuedItem(const endpoint_type& endpoint, 
            ostreambuf_type& streambuf,
            key_type key) :
            estd::experimental::forward_node_base_base<QueuedItem*>(nullptr),
            endpoint(endpoint),
            streambuf(streambuf),
            retry_count(0),
            key(key)
        {

        }
    };

    typedef estd::intrusive_forward_list<QueuedItem> list_type;
    typedef typename list_type::iterator list_iterator;
    list_type items;

    void send(const endpoint_type& to, ostreambuf_type& streambuf, key_type key)
    {
        // NOTE: Don't like dynamic allocation, it's kinda the norm for
        // FreeRTOS apparently.  
        // TODO: At least use allocator_traits and friends so that we can sub in
        // memory pools and the like
        QueuedItem* item = new QueuedItem(to, streambuf, key);

        items.push_front(*item);
    }


    void send(const endpoint_type& to, ostreambuf_type& streambuf)
    {
        key_type key = extract_key(streambuf);

        send(to, streambuf, key);
    }

    // Shall need an app-specific identifier, endpoint alone is not enough
    // (generally) to distinguish whether this is the specific item in question
    // streambuf will need inspection (for CoAP, we'll be looking for MID)
    void evaluate_received(const endpoint_type& from, istreambuf_type& streambuf)
    {
        key_type key = extract_key(streambuf);

        evaluate_received(from, key);
    }

    void evaluate_received(const endpoint_type& from, key_type key)
    {
        list_iterator found = estd::find_if(items.first(), items.last(),
            [&](const QueuedItem& item)  
            {
                // policy impl helps for IP to compare only addr part, not port part
                bool addr_match = policy_impl.match(from, item.endpoint);
                return addr_match && item.key == key; 
            });
    }
};


}}