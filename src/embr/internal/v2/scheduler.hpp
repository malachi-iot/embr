#pragma once

#include "scheduler.h"

namespace embr { namespace scheduler { inline namespace v1 {

namespace detail {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
template <class Mutex>
auto scheduler<Traits, Container>::process_one(time_point now, Mutex mutex) -> process_result
{
    mutex.lock();

    if(items_.empty())
    {
        mutex.unlock();

        return UNPROCESSED;
    }

    pointer t = items_.top();
    const time_point next = traits::next(*t);

    // Are we actually at a next up situation?
    if(now >= next)
    {
        items_.pop();

        mutex.unlock();

        // DEBT: Shouldn't we have an immediate-re-eval signal here?
        t->process(now);

        // Now that we've processed, next may have changed.  If we have something
        // to reschedule, do so
        // NOTE: This is so far the only signal a reschedule is desired, a differing next.
        if(traits::next(*t) != next)
        {
            mutex.lock();
            items_.push(t);
            mutex.unlock();
            return PROCESSED_AND_RESCHEDULED;
        }

        return PROCESSED;
    }
    else
        mutex.unlock();

    return UNPROCESSED;
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
void scheduler<Traits, Container>::process(time_point now)
{
    process_one(now);
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
template <class Mutex>
bool scheduler<Traits, Container>::reschedule(pointer v, Mutex mutex)
{
    mutex.lock();
    bool erased = items_.erase_if([v](pointer item) { return v == item; });
    items_.push(v);
    mutex.unlock();
    return erased;
}

}

}}}
