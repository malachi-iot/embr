#pragma once

#include "timer-scheduler.h"
#include "../../exp/runtime-chrono.h"

namespace embr { namespace esp_idf {


static auto last_now = estd::chrono::esp_clock::now();

template <class TScheduler>
bool IRAM_ATTR DurationImpl::helper<TScheduler>::timer_callback (void* arg)
{
    TScheduler& scheduler = * (TScheduler*) arg;
    uint64_t counter;

    //ets_printf("0 TimerScheduler Intr: group=%d, idx=%d\n", timer.group, timer.idx);

    counter = scheduler.timer().get_counter_value_in_isr();

    auto now = estd::chrono::esp_clock::now();
    auto duration = now - last_now;

    ets_printf("1 TimerScheduler Intr: duration=%lldus, timer_counter=%lld, scheduler=%p\n",
        duration.count(), counter, &scheduler);

    // DEBT: Don't do auto here
    // DEBT: Pass in something like 'in_isr_tag' do disambiguate inputs to create_context
    auto context = scheduler.create_context(true);  // create an in_isr context

    context.higherPriorityTaskWoken = false;

    scheduler.process(time_point(counter), context);

    // DEBT: Consider making a scheduler.empty()
    // DEBT: We need mutex protection down here too
    bool more = scheduler.size() > 0;
    if(more)
    {
        time_point next = scheduler.top_time();

        embr::experimental::TimerSchedulerConverter converter;

        uint64_t native = converter.convert(next);

        ets_printf("1.1 TimerScheduler Intr: size=%d, next=%lluus\n",
            scheduler.size(),
            next.count());

        scheduler.timer().set_alarm_value_in_isr(native);
    }
    else
    {
        ets_printf("1.2 TimerScheduler Intr: no further events\n");
        //timer.pause();
    }

    // Recommended by
    // https://www.freertos.org/a00124.html
    portYIELD_FROM_ISR(context.higherPriorityTaskWoken);

    // DEBT: Document what this does - I think it relates to the portYIELD mechanism above
    return false;
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
void DurationImpl::init(TScheduler* scheduler)
{
    // Set prescaler for 1 MHz clock - remember, we're dividing
    // "default is APB_CLK running at 80 MHz"
    uint32_t divider = 80;

    timer_scheduler_init(timer(), divider, &helper<TScheduler>::timer_callback, scheduler);
}

template <class TScheduler>
inline void TimerScheduler<TScheduler>::schedule(value_type& v)
{
    const char* TAG = "TimerScheduler::schedule";

    //timer.enable_alarm_in_isr();
    time_point t = impl_type::get_time_point(v);

    embr::experimental::TimerSchedulerConverter converter;

    uint64_t native = converter.convert(t);

    scheduler.schedule(v);

    last_now = estd::chrono::esp_clock::now();

    if(scheduler.size() == 1)
    {
        timer().set_alarm(TIMER_ALARM_EN);
        timer().set_alarm_value(native);
        timer().start();
    }

    ESP_LOGD(TAG, "group=%d, idx=%d, scheduled=%llu", timer().group, timer().idx, native);
}


}} // namespace embr { namespace esp_idf 