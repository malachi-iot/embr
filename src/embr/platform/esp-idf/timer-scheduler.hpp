#pragma once

#include "timer-scheduler.h"
#include "../../exp/runtime-chrono.h"

namespace embr { namespace esp_idf {

// DEBT: Move this out of static/global namespace and conditionally compile only for
// diagnostic scenarios
static auto last_now_diagnostic = estd::chrono::esp_clock::now();

template <class TScheduler>
bool IRAM_ATTR DurationImplBaseBase::helper<TScheduler>::timer_callback (void* arg)
{
    TScheduler& scheduler = * (TScheduler*) arg;
    Timer& timer = scheduler.timer();
    typedef typename TScheduler::time_point time_point;
    uint64_t counter;

    //ets_printf("0 TimerScheduler Intr: group=%d, idx=%d\n", timer.group, timer.idx);

    counter = scheduler.timer().get_counter_value_in_isr();

    auto now = estd::chrono::esp_clock::now();
    auto duration = now - last_now_diagnostic;

    ESP_DRAM_LOGD(TAG, "timer_callback: [%d:%d] (%p) duration=%lldus, timer_counter=%lld",
        timer.group, timer.idx, &scheduler,
        duration.count(), counter);

    // DEBT: Don't do auto here
    // DEBT: Pass in something like 'in_isr_tag' do disambiguate inputs to create_context
    auto context = scheduler.create_context(true);  // create an in_isr context

    // DEBT: Pretty sure our context_factory handles this now, but keeping around until 100% sure
    context.higherPriorityTaskWoken = false;

    // convert native esp32 Timer format back to scheduler format
    time_point current_time;
    scheduler.duration_converter().convert(counter, &current_time);

    scheduler.process(current_time, context);

    // DEBT: We need mutex protection down here too
    if(!scheduler.empty())
    {
        time_point next = scheduler.top_time();

        uint64_t native = scheduler.duration_converter().convert(next);

        ESP_DRAM_LOGD(TAG, "timer_callback: size=%d, next=%llu / %llu(ticks)",
            scheduler.size(),
            next.count(),
            native);

        timer.set_alarm_value_in_isr(native);
        timer.start();
    }
    else
    {
        ESP_DRAM_LOGD(TAG, "timer_callback: no further events");

        // Can't do this because this refers only to initial counter value
        //timer.set_counter_value(0);
        timer.pause();
    }

    // "The callback should return a bool value to determine whether need to do YIELD at the end of the ISR."
    // effectively https://www.freertos.org/a00124.html
    return context.higherPriorityTaskWoken;
}

/*
template <class TScheduler>
bool IRAM_ATTR DurationImpl::timer_callback (void *param)
{
    //timer.pause();

    static_cast<this_type*>(param)->timer_callback();

    // DEBT: Pretty sure we need an ISR-friendly version of this.  However, I can't find one
    //timer_set_alarm(timer_group, timer_idx, TIMER_ALARM_DIS);
    // Above not needed, becase
    // "Once triggered, the alarm is disabled automatically and needs to be re-enabled to trigger again."

    return false;
}
*/

template <class TScheduler>
void DurationImplBaseBase::init(TScheduler* scheduler, uint32_t divider)
{
    timer_scheduler_init(timer(), divider, &helper<TScheduler>::timer_callback, scheduler);
}

/*
template <class TScheduler>
inline void DurationImpl::on_scheduling(value_type& value,
    embr::internal::SchedulerContextBase<TScheduler>& context)
{
    // Putting this in here rather than scheduled because it makes timestamps look more accurate.
    // Didn't dig into the whys and hows at all
    last_now_diagnostic = estd::chrono::esp_clock::now();
}


// NOTE: All this happens while in the locked mutex 
template <class TScheduler>
inline void DurationImpl::on_scheduled(const value_type& value,
    embr::internal::SchedulerContextBase<TScheduler>& context)
{
    time_point t = get_time_point(value);

    ESP_LOGV(TAG, "on_scheduled: entry");
    ESP_LOGD(TAG, "on_scheduled: group=%d, idx=%d, scheduled=%u", timer().group, timer().idx,
        t.count());

    uint64_t native = duration_converter().convert(t);

    if(context.scheduler().size() == 1)
    {
        timer().set_alarm(TIMER_ALARM_EN);
        timer().set_alarm_value(native);
        timer().start();
    }
}
*/

template <class T, int divider_, typename TTimePoint, class TReference>
template <class TScheduler>
inline void DurationImpl2<T, divider_, TTimePoint, TReference>::on_scheduled(
    const value_type& value, const scheduler_context_type<TScheduler>& context)
{
    ESP_DRAM_LOGV(TAG, "on_scheduled: entry");
    
    time_point t = get_time_point(value);
    uint64_t native = duration_converter().convert(t);

    // DEBT: Do we need a non-chrono version?
    ESP_DRAM_LOGD(TAG, "on_scheduled: group=%d, idx=%d, scheduled=%llu / %llu(ticks)", timer().group, timer().idx,
        t.count(),
        native);

    if(context.is_processing())
    {

    }

    // DEBT: Works, but sloppy - clean up semi-lazy init of timer when outside ISR
    if(context.scheduler().size() == 1 && !context.in_isr())
    {
        timer().set_alarm(TIMER_ALARM_EN);
        timer().set_alarm_value(native);
    }

    timer().start();
}

template <class T, int divider_, typename TTimePoint, class TReference>
template <class TScheduler>
inline void DurationImpl2<T, divider_, TTimePoint, TReference>::on_processed(
    const value_type* value, time_point t, const scheduler_context_type<TScheduler>& context)
{
    if(value == nullptr)
    {
        // we finished a batch of processing and arrive here

        ESP_DRAM_LOGD(TAG, "on_processed: finished processing - count=%d",
            context.scheduler().size());
    }
}

/*
template <class TScheduler>
inline void TimerScheduler<TScheduler>::schedule(value_type& v)
{
    //timer.enable_alarm_in_isr();
    scheduler.schedule(v);
}
*/


}} // namespace embr { namespace esp_idf 