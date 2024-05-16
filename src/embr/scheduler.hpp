#pragma once

#include "scheduler.h"

namespace embr { namespace internal {

template <class TContainer, class TImpl, class TSubject>
template <class TContext>
bool Scheduler<TContainer, TImpl, TSubject>::process_one(time_point current_time, context_type<TContext>& context)
{
    context.is_processing(true);

    scoped_lock<accessor> t(top());
    time_point eval_time = impl().get_time_point(*t);

    // If current time is ahead of what's queued/prioritized
    if(current_time >= eval_time)
    {
        // Then we have an item to process
        do_notify_processing(*t, current_time, context);

        bool reschedule_requested = process_impl(*t, current_time, context);

        do_notify_processed(t.get(), current_time, context);

        // Need to do this because *t is a pointer into event_queue and the pop moves
        // items around.
        value_type copied = *t;

        event_queue.pop();

        subject_provider::value().notify(removed_event_type (copied), context);

        if(reschedule_requested)
        {
            // DEBT: Doesn't handle move variant
            schedule_internal(copied, context);
        }

        // Processed this item and potentially more are awaiting, denoted by true
        return true;
    }
    else
    {
        // No item to process, so denote completion with nullptr && false return

        do_notify_processed(nullptr, current_time, context);
        context.is_processing(false);
        return false;
    }
}

template <class TContainer, class TImpl, class TSubject>
template <class TContext>
void Scheduler<TContainer, TImpl, TSubject>::process(time_point current_time, context_type<TContext>& context)
{
    mutex_guard m(context);

    while(!event_queue.empty())
    {
        if(process_one(current_time, context) == false)
            return;
    }

    // DEBT: Clean this up via a wrapper/internal call
    do_notify_processed(nullptr, current_time, context);
    context.is_processing(false);
}


}}