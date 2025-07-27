#pragma once

#include "scheduler.h"

namespace embr { namespace scheduler { inline namespace v1 {

namespace detail {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits>
bool scheduler<Traits>::process_one(time_point now)
{
    if(items_.empty())  return false;

    pointer t = items_.top();
    const time_point next = traits::next(*t);

    // Are we actually at a next up situation?
    if(now >= next)
    {
        items_.pop();
        t->process();

        // Now that we've processed, next may have changed.  If we have something
        // to reschedule, do so
        // NOTE: This is so far the only signal a reschedule is desired, a differing next.
        if(traits::next(*t) != next)
        {
            items_.push(t);
            return true;
        }
    }

    return false;
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits>
void scheduler<Traits>::process(time_point now)
{
    process_one(now);
}

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits>
bool scheduler<Traits>::reschedule(pointer v)
{
    bool erased = items_.erase_if([v](pointer item) { return v == item; });
    items_.push(v);
    return erased;
}

}

}}}
