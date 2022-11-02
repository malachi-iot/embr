#pragma once

#include <estd/string.h>

#include "timer-scheduler.h"
#include "../../scheduler.hpp"
#include "../../exp/runtime-chrono.h"

#include "log.h"

#define EMBR_ESP_IDF_TIMER_PROFILING CONFIG_EMBR_ESP_IDF_TIMER_PROFILING

namespace embr { namespace esp_idf {

// DEBT: Would like to do a const_string here, but estd conversion/availability
// for a layer1 const_string seems to be impacted
inline estd::layer1::string<8> to_string(const Timer& timer)
{
    estd::layer1::string<8> timer_str;

    timer_str += estd::to_string((int)timer.group);
    timer_str += ':';
    timer_str += estd::to_string((int)timer.idx);

    return timer_str;
}

// DEBT: Move this out of static/global namespace and conditionally compile only for
// diagnostic scenarios
static auto last_now_diagnostic = estd::chrono::esp_clock::now();

template <class TScheduler>
inline bool IRAM_ATTR DurationImplBaseBase::timer_callback (TScheduler& scheduler)
{
    return false;
}

template <class TScheduler>
inline bool IRAM_ATTR DurationImplBaseBase::Wrapper<TScheduler>::timer_callback()
{
    constexpr const char* TAG = "Wrapper::timer_callback";

    auto& q = this_type::event_queue;

    ESP_DRAM_LOGV(TAG, "event_queue size=%d", q.size());

    return false;
}


template <class TScheduler>
bool IRAM_ATTR DurationImplBaseBase::timer_callback (void* arg)
{
    TScheduler& scheduler = * (TScheduler*) arg;
    Timer& timer = scheduler.timer();
    typedef typename TScheduler::time_point time_point;
    uint64_t initial_counter = timer.get_counter_value_in_isr();
    uint64_t counter = initial_counter;
    counter -= scheduler.offset;
    // Get maximum number of timer ticks we can accumulate before rolling over scheduler's timebase
    // NOTE: counter itself we don't worry about it rolling over, even at highest precision it would take thousands
    // of years for it to do so
    uint64_t max = scheduler.duration_converter().convert(time_point::max());
    estd::layer1::string<8> timer_str = to_string(timer);

    scheduler.timer_callback(scheduler);

    // It works!  Should we do it.... ?
    static_cast<Wrapper<TScheduler>*>(arg)->timer_callback();

    // Switching from verbose mode to regular debug goes from ~40-60ms to < 15ms
    // This #if goes from 13-14.5ms to 11-12.5ms
#if DISABLED_WHILE_DIAGNOSING_SPEED
    ESP_DRAM_LOGV(TAG, "timer_callback: [%s] (%p) offset=%llu, max=%llu", timer_str,
        &scheduler,
        scheduler.offset,
        max);
    auto now = estd::chrono::esp_clock::now();
    auto duration = now - last_now_diagnostic;

    ESP_DRAM_LOGD(TAG, "timer_callback: [%s] duration=%lldus, counter(ticks)=%lld",
        timer_str, duration.count(), counter);
#else
    ESP_GROUP_LOGD(1, TAG, "timer_callback: [%s] counter(ticks)=%lld",
        timer_str.data(), counter);
#endif

    // DEBT: Don't do auto here
    // DEBT: Pass in something like 'in_isr_tag' do disambiguate inputs to create_context
    auto context = scheduler.create_context(true);  // create an in_isr context

    // DEBT: Pretty sure our context_factory handles this now, but keeping around until 100% sure
    context.higherPriorityTaskWoken = false;

    uint64_t native_now;
#if EMBR_ESP_IDF_TIMER_PROFILING
    int debug_counter1, debug_counter2, debug_counter3;
    
    native_now = scheduler.timer().get_counter_value_in_isr();

    debug_counter1 = native_now - initial_counter;
#endif

    // convert native esp32 Timer format back to scheduler format
    time_point current_time;
    if(counter > max)
    {
        // DEBT: Somehow my wrapper doesn't autocast timer_str to const char*
        ESP_GROUP_LOGI(1, TAG, "timer_callback: [%s], overflow", timer_str.data());
    }
    scheduler.duration_converter().convert(counter, &current_time);

    scheduler.process(current_time, context);

    native_now = scheduler.timer().get_counter_value_in_isr();

#if EMBR_ESP_IDF_TIMER_PROFILING
    debug_counter2 = native_now - initial_counter;
#endif

    // DEBT: We need mutex protection down here too
    if(!scheduler.empty())
    {
        time_point next = scheduler.top_time();

        uint64_t native = scheduler.duration_converter().convert(next);

        ESP_GROUP_LOGD(1, TAG, "timer_callback: size=%d, next=%llu / %llu(ticks), native_now=%llu",
            scheduler.size(),
            next.count(),
            native,
            native_now);

        native += scheduler.offset;

        timer.set_alarm_value_in_isr(native);
        timer.start();
    }
    else
    {
        // fake-zero it out
        // grab time again because ISR service can be slow when debugging
        scheduler.offset = native_now;
        //scheduler.offset = initial_counter;   // DEBT: Bring this back in if debug level is minimal

        ESP_GROUP_LOGD(1, TAG, "timer_callback: no further events: offset=%llu", scheduler.offset);

        // Can't do this because this refers only to initial counter value
        //timer.set_counter_value(0);
        timer.pause();
    }

#if EMBR_ESP_IDF_TIMER_PROFILING
    native_now = scheduler.timer().get_counter_value_in_isr();

    debug_counter3 = native_now - initial_counter;

    ESP_DRAM_LOGI(TAG, "timer_callback: debug_counter1=%d, debug_counter2=%d, debug_counter3=%d",
        debug_counter1,
        debug_counter2,
        debug_counter3);
#endif

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
inline void DurationImplBaseBase::start(TScheduler* scheduler, uint32_t divider)
{
    timer_scheduler_init(timer(), divider, &DurationImplBaseBase::timer_callback<TScheduler>, scheduler);
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

    auto timer_str = to_string(timer());

    // DEBT: Do we need a non-chrono version?
    ESP_DRAM_LOGD(TAG, "on_scheduled: [%s], scheduled=%llu / %llu(ticks)", timer_str,
        t.count(),
        native);

    native += context.scheduler().offset;

    if(context.is_processing())
    {

    }

    // DEBT: Works, but sloppy - clean up semi-lazy init of timer when outside ISR
    if(context.scheduler().size() == 1 && !context.in_isr())
    {
        ESP_DRAM_LOGV(TAG, "on_scheduled: setting alarm=%llu(ticks)", native);

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

        ESP_DRAM_LOGV(TAG, "on_processed: finished processing - count=%d",
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