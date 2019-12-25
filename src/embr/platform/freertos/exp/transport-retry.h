/**
 * @file
 *  Experimental 
 *  Informally a retry-v3
 *  FreeRTOS-specific retry queue implementation
 *  Presumes datagram-style behavior:
 *  1) sessionless, no inbuilt sequence mechanism
 *  2) non blocking/async
 * 
 *  Also depends heavily on estd::streambuf
 */

#include <estd/internal/platform.h>

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#else
#include <timers.h>
#endif

namespace embr { namespace experimental {

// We expect TTransport to be conforming to something like ../lwip/transport.h
//    an important component of that is that it accepts a TStreambuf for input
// We expect TRetryPolicyImpl to help us choose how long retry delays are as well
//    as how many times to retry
template <class TTransport, class TRetryPolicyImpl>
struct RetryManager
{
    TRetryPolicyImpl policy_impl;

    typedef typename TTransport::endpoint_type endpoint_type;
    typedef typename TTransport::ostreambuf_type ostreambuf_type;

    struct QueuedItem
    {
        endpoint_type endpoint;
        ostreambuf_type& streambuf;
        unsigned retry_count;
    };


    void add(ostreambuf_type& streambuf, const endpoint_type& endpoint)
    {

    }
};


}}