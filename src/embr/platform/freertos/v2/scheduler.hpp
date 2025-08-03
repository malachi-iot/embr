#pragma once

#include "scheduler.h"

// 03AUG25 MB DEBT: v1 namespace living in v2 folder

namespace embr { namespace scheduler { namespace freertos { inline namespace v1 {

namespace detail {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
BaseType_t scheduler_with_notify<Traits, Container>::process_one(duration timeout)
{
    mutex_.lock();
    if(base_type::empty())
    {
        mutex_.unlock();
        return pdFALSE;
    }

    const_reference top = base_type::top();

    mutex_.unlock();

    uint32_t v;
    const time_point now = clock_type::now();

    if(now >= top.next())
    {
        // Pseudo-race condition possible: someone can slide in a scheduled item
        // before this guy.  However, process_one handles that gracefully.
        process_result r = base_type::process_one(now, mutex_);
        return pdFALSE;
    }

    duration interval = estd::min(timeout, top.next() - now);

    BaseType_t r = xTaskNotifyWaitIndexed(0, 0, 0, &v, interval.count());

    base_type::process_one(now, mutex_);

    return r;
}


}

}}}}
