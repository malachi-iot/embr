#pragma once

#include "scheduler.h"

namespace embr { namespace internal {

template <class TContainer, class TImpl, class TSubject>
template <class TContext>
void Scheduler<TContainer, TImpl, TSubject>::process(time_point current_time, context_type<TContext>& context)
{
    mutex_guard m(context);

    while(!event_queue.empty())
    {
        context.is_processing(true);

        scoped_lock<accessor> t(top());
        time_point eval_time = impl().get_time_point(*t);

        if(current_time >= eval_time)
        {
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
        }
        else
        {
            do_notify_processed(nullptr, current_time, context);
            context.is_processing(false);
            return;
        }
    }

    // DEBT: Clean this up via a wrapper/internal call
    do_notify_processed(nullptr, current_time, context);
    context.is_processing(false);
}


}}