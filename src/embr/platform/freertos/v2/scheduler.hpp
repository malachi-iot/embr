#pragma once

#include "scheduler.h"

// 03AUG25 MB DEBT: v1 namespace living in v2

namespace embr { namespace scheduler { namespace freertos { inline namespace v1 {

namespace detail {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
auto scheduler_with_notify<Traits, Container>::process_one(duration timeout) -> process_result
{
    const time_point now = clock_type::now();
    process_result r1 = base_type::process_one(now, mutex_);

    if(r1 != process_result::UNPROCESSED)    return r1;

    mutex_.lock();

    duration interval = estd::min(timeout, base_type::next() - now);

    mutex_.unlock();

    // It's possible for someone to sneak in and schedule something in this sliver of
    // time.  In that event, notify has our back and queues up, thus shaking us out
    // of wait immediately.  In this case, process_one MAY return UNPROCESSED

    uint32_t v;

    [[maybe_unused]]
    BaseType_t r = xTaskNotifyWaitIndexed(0, 0, 0, &v, interval.count());

    // r = pdTRUE when scheduling interrupted this wait
    // r = pdFALSE when full callee-specified timeout transpired
    // DEBT: Would be good to heed 'r' and augment r1 with it

    r1 = base_type::process_one(clock_type::now(), mutex_);

    return r1;
}


}

}}}}
