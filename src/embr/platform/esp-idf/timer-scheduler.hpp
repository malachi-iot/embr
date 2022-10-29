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

    ets_printf("1 TimerScheduler Intr: [%d:%d] duration=%lldus, timer_counter=%lld, scheduler=%p\n",
        timer.group, timer.idx,
        duration.count(), counter, &scheduler);

    // DEBT: Don't do auto here
    // DEBT: Pass in something like 'in_isr_tag' do disambiguate inputs to create_context
    auto context = scheduler.create_context(true);  // create an in_isr context

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

        ets_printf("1.1 TimerScheduler Intr: size=%d, next=%llu / %llu(ticks)\n",
            scheduler.size(),
            next.count(),
            native);

        scheduler.timer().set_alarm_value_in_isr(native);
    }
    else
    {
        ets_printf("1.2 TimerScheduler Intr: no further events\n");
        //timer.pause();
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

template <class TScheduler>
inline void DurationImpl::on_scheduling(value_type& value,
    internal::SchedulerContextBase<TScheduler>& context)
{
    // Putting this in here rather than scheduled because it makes timestamps look more accurate.
    // Didn't dig into the whys and hows at all
    last_now_diagnostic = estd::chrono::esp_clock::now();
}


// NOTE: All this happens while in the locked mutex 
template <class TScheduler>
inline void DurationImpl::on_scheduled(const value_type& value, internal::SchedulerContextBase<TScheduler>& context)
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


template <class T, int divider_, typename TTimePoint, class TReference>
template <class TScheduler>
inline void DurationImpl2<T, divider_, TTimePoint, TReference>::on_scheduled(
    const value_type& value,
    const internal::SchedulerContextBase<TScheduler>& context)
{
    ESP_DRAM_LOGV(TAG, "on_scheduled: entry");
    
    time_point t = get_time_point(value);
    uint64_t native = duration_converter().convert(t);

    // DEBT: Do we need a non-chrono version?
    ESP_DRAM_LOGD(TAG, "on_scheduled: group=%d, idx=%d, scheduled=%llu / %llu(ticks)", timer().group, timer().idx,
        t.count(),
        native);

    // DEBT: Works, but sloppy - clean up semi-lazy init of timer when outside ISR
    if(context.scheduler().size() == 1 && !context.in_isr())
    {
        timer().set_alarm(TIMER_ALARM_EN);
        timer().set_alarm_value(native);
        timer().start();
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