#pragma once

#include "timer.h"
#include "timer-scheduler.h"
#include "../../scheduler.hpp"

#include "log.h"

// TODO: Do something like https://en.cppreference.com/w/cpp/chrono/duration/operator_ltlt
// to more comfortably feed durations into logger

#define EMBR_ESP_IDF_TIMER_PROFILING CONFIG_EMBR_ESP_IDF_TIMER_PROFILING

namespace embr { namespace scheduler { namespace esp_idf { namespace impl {

#define EMBR_LOG_GROUP_ISR 1

//ASSERT_EMBR_LOG_GROUP_MODE(EMBR_LOG_GROUP_ISR, EMBR_LOG_GROUP_MODE_ISR)
ASSERT_EMBR_LOG_GROUP_MODE(1, EMBR_LOG_GROUP_MODE_ISR)

static constexpr int isr_log_group = EMBR_LOG_GROUP_ISR;

// DEBT: Move this out of static/global namespace and conditionally compile only for
// diagnostic scenarios
static auto last_now_diagnostic = estd::chrono::esp_clock::now();

template <class TScheduler>
inline bool IRAM_ATTR TimerBase::timer_callback (TScheduler& scheduler)
{
    return false;
}

template <class TScheduler>
inline bool IRAM_ATTR TimerBase::Wrapper<TScheduler>::timer_callback()
{
    constexpr const char* TAG = "Wrapper::timer_callback";

    auto& q = this_type::event_queue;

    ESP_GROUP_LOGV(1, TAG, "event_queue size=%d", q.size());

    return false;
}

template <class TScheduler, class TDuration>
inline void TimerBase::rebase(TScheduler& scheduler, TDuration next)
{
    constexpr const char* TAG = "TimerBase::rebase";

    ESP_GROUP_LOGV(1, TAG, "entry: next=%llu", (uint64_t)next.count());

    typedef TScheduler scheduler_type;
    typedef Buddy<scheduler_type> buddy_type;
    // This works, but we don't use it which makes gcc mad.
    // DEBT: Use this instead of passing in TDuration, which means we are making 'duration' an official
    // member of impl/scheduler which I think is a good idea
    //typedef typename scheduler_type::duration duration;
    typedef typename buddy_type::container_type container_type;
    typedef typename embr::internal::Rebaser<container_type> rebaser_type;

    container_type& c = buddy_type::container(scheduler);

    rebaser_type r(c);
    
    r.rebase(next);
}


template <class TScheduler>
bool IRAM_ATTR TimerBase::timer_callback (void* arg)
{
    TScheduler& scheduler = * (TScheduler*) arg;
    timer_type& timer = scheduler.timer();
    typedef typename TScheduler::time_point time_point;
    uint64_t native_now;
    uint64_t initial_counter = timer.get_counter_value_in_isr();
    // DEBT: 'counter' needs better name, something like native_now_offset
    uint64_t counter = initial_counter;
    counter -= scheduler.native_offset;
    // NOTE: counter itself we don't worry about it rolling over, even at highest precision it would take thousands
    // of years for it to do so
    const uint64_t max = scheduler.overflow_max();
    estd::layer1::string<8> timer_str = to_string(timer);

    scheduler.timer_callback(scheduler);

    // It works!  Should we do it.... ?
    auto wrapper = static_cast<Wrapper<TScheduler>*>(arg);

    wrapper->timer_callback();

    // DEBT: Don't do auto here
    // DEBT: Pass in something like 'in_isr_tag' do disambiguate inputs to create_context
    // DEBT: We disable mutex as is appropriate since we handle it ourselves, but it's confusing
    // that this context still locks/unlocks when manually activated
    auto context = scheduler.create_context(true, false);  // create an in_isr context, but disable mutex anyway since we manually manage it

    // DEBT: Pretty sure our context_factory handles this now, but keeping around until 100% sure
    context.higherPriorityTaskWoken = false;

    scheduler.isr_acquiring_mutex_ = true;

    // mutex isn't supported in ISR, we need a binary semaphore.  
    // mutex does actually function as a binary sempahore here, as per
    // embr::scheduler::freertos::timed_mutex<true, ...>
    auto result = scheduler.mutex().native_handle().take_from_isr(&context.higherPriorityTaskWoken);

    // DEBT: Unknown implications for repeated calls to this when already clear.
    // Likely nothing but a bit of queue overhead, but we should be sure
    scheduler.event_.clear_bits_from_isr(event_clear_to_schedule);

    if(result == pdFALSE)
    {
        ESP_GROUP_LOGD(1, TAG, "timer_callback: [%s] could not grab mutex native=%llu",
            timer_str.data(),
            initial_counter);

        // Effectively do a spinwait

        // FIX: This huge value is needed because otherwise ISR lights right back up again
        // due to slowness incurred by debug messages, subsequently triggering WDT (appropriately)
        // this is despite ESP_INTR_FLAG_LEVEL1 which is the lowest priority interrupt
        //timer.set_alarm_value_in_isr(initial_counter + 100000);

        // DEBT: slightly better than above FIX - wake up 1000 ticks after this point in time,
        // generally accounting for debug log slowness.  See 'get_spinwait_wakeup'
        native_now = timer.get_counter_value_in_isr();
        timer.set_alarm_value_in_isr(scheduler.get_spinwait_wakeup(native_now));

        // NOTE: This return true doesn't help above WDT issue, but keeping it for now anyway
        return true;
    }

    scheduler.isr_acquiring_mutex_ = false;
    
    // Switching from verbose mode to regular debug goes from ~40-60ms to < 15ms
    // This #if goes from 13-14.5ms to 11-12.5ms
#if DISABLED_WHILE_DIAGNOSING_SPEED
    ESP_DRAM_LOGV(TAG, "timer_callback: [%s] (%p) offset=%llu, max=%llu", timer_str,
        &scheduler,
        scheduler.native_offset,
        max);
    auto now = estd::chrono::esp_clock::now();
    auto duration = now - last_now_diagnostic;

    ESP_DRAM_LOGD(TAG, "timer_callback: [%s] duration=%lldus, counter(ticks)=%lld",
        timer_str, duration.count(), counter);
#else
    ESP_GROUP_LOGD(1, TAG, "timer_callback: [%s] counter(ticks)=%lld",
        timer_str.data(), counter);
#endif

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
        ESP_GROUP_LOGI(1, TAG, "timer_callback: [%s] overflow max(time_point)=%llu, max(ticks)=%llu", 
            timer_str.data(),
            (uint64_t)time_point::max().count(),
            max);
    }

    scheduler.duration_converter().convert(counter, &current_time);

    scheduler.process(current_time, context);

    native_now = scheduler.timer().get_counter_value_in_isr();

#if EMBR_ESP_IDF_TIMER_PROFILING
    debug_counter2 = native_now - initial_counter;
#endif

    // Remember, all this is protected by 'mutex' aka binary semaphore
    if(!scheduler.empty())
    {
        time_point next = scheduler.top_time();

        uint64_t native_next = scheduler.duration_converter().convert(next);

        ESP_GROUP_LOGD(1, TAG, "timer_callback: size=%d, next=%llu / %llu(ticks), native_now=%llu",
            scheduler.size(),
            (uint64_t)next.count(),
            native_next,
            native_now);

        // Estimate if we are coming at risk of overflow (of time_point, not uint64_t)
        // DEBT: Need a much more precise and configrable mechanism here
        if(native_next > max / 2)
        {
            // LIGHTLY TESTED

            // yank all scheduled values by current_time - remember, 'next' is still ahead of current_time so
            // there's still space to schedule something beforehand
            scheduler.rebase(scheduler, current_time);

            native_next += scheduler.native_offset;        // native_next is an absolute value, so current offset is still accurate
            scheduler.native_offset = initial_counter;          // fake zero out our native counter
        }
        else
            native_next += scheduler.native_offset;

        timer.set_alarm_value_in_isr(native_next);
        //timer.start();
    }
    else
    {
        // fake-zero it out
        // grab time again because ISR service can be slow when debugging
        scheduler.native_offset = native_now;
        //scheduler.offset = initial_counter;   // DEBT: Bring this back in if debug level is minimal

        ESP_GROUP_LOGD(1, TAG, "timer_callback: no further events: offset=%llu", scheduler.native_offset);

        // Can't do this because this refers only to initial counter value
        //timer.set_counter_value(0);
        timer.pause();
#if EMBR_TIMER_TRACK_START
        scheduler.is_timer_started_ = false;
#endif
    }

#if EMBR_ESP_IDF_TIMER_PROFILING
    native_now = scheduler.timer().get_counter_value_in_isr();

    debug_counter3 = native_now - initial_counter;

    ESP_DRAM_LOGI(TAG, "timer_callback: debug_counter1=%d, debug_counter2=%d, debug_counter3=%d",
        debug_counter1,
        debug_counter2,
        debug_counter3);
#endif

    // DEBT: Double check that 'higherPriorityTaskWoken' doesn't get clobbered by both a mutex unlock
    // and a set_bits_from_isr
    scheduler.event_.set_bits_from_isr(event_clear_to_schedule, &context.higherPriorityTaskWoken);
    scheduler.mutex().unlock(context);

    // "The callback should return a bool value to determine whether need to do YIELD at the end of the ISR."
    // effectively https://www.freertos.org/a00124.html
    return context.higherPriorityTaskWoken;
}


template <class TScheduler>
inline void TimerBase::start(TScheduler* scheduler, uint32_t divider)
{
    event_.set_bits(event_clear_to_schedule);

    timer_scheduler_init(timer(), divider, &TimerBase::timer_callback<TScheduler>, scheduler);
}


template <class T, int divider_, typename TTimePoint, class TReference>
template <class TScheduler>
inline void Timer<T, divider_, TTimePoint, TReference>::on_scheduled(
    const value_type& value, const scheduler_context_type<TScheduler>& context)
{
    ESP_GROUP_LOGV(1, TAG, "on_scheduled: entry");
    
    time_point t = get_time_point(value);
    uint64_t native = duration_converter().convert(t);

    auto timer_str = to_string(timer());

    // DEBT: Do we need a non-chrono version?
    ESP_GROUP_LOGD(1, TAG, "on_scheduled: [%s], scheduled=%llu / %llu(ticks)", timer_str.data(),
        (uint64_t)t.count(),
        native);

    native += context.scheduler().native_offset;

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

#if EMBR_TIMER_TRACK_START
    if(is_timer_started_) return;
#endif

    timer().start();
#if EMBR_TIMER_TRACK_START
    is_timer_started_ = true;
#endif
}

template <class T, int divider_, typename TTimePoint, class TReference>
template <class TScheduler>
inline void Timer<T, divider_, TTimePoint, TReference>::on_processed(
    const value_type* value, time_point t, const scheduler_context_type<TScheduler>& context)
{
    if(value == nullptr)
    {
        // we finished a batch of processing and arrive here

        ESP_GROUP_LOGV(1, TAG, "on_processed: finished processing - count=%d",
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


}}}}