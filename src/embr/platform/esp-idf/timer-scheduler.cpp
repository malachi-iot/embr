#if defined(ESP_PLATFORM)

#include <esp_log.h>

#include "v1/scheduler.hpp"

namespace embr { namespace esp_idf {

#if FEATURE_EMBR_ESP_TIMER_SCHEDULER

#include "timer-scheduler.hpp"

void timer_scheduler_init(Timer& timer, uint32_t divider, timer_isr_t isr_handler, void* arg)
{
    const char* TAG = "timer_scheduler_init";

    ESP_LOGI(TAG, "group=%d, idx=%d, arg=%p", timer.group, timer.idx, arg);

    timer_config_t config;

    config.divider = divider; 

    config.counter_dir = TIMER_COUNT_UP;
    config.alarm_en = TIMER_ALARM_DIS;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = TIMER_AUTORELOAD_DIS;
    //config.auto_reload = TIMER_AUTORELOAD_EN; // Reset timer to 0 when end condition is triggered
    config.counter_en = TIMER_PAUSE;
#if SOC_TIMER_GROUP_SUPPORT_XTAL || SOC_TIMER_GROUP_SUPPORT_APB
    config.clk_src = TIMER_SRC_CLK_APB;
#endif
    timer.init(&config);

    timer.set_counter_value(0);
    timer.isr_callback_add(isr_handler, arg, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM);
    timer.enable_intr();
}

#endif

}}

namespace embr { namespace scheduler { namespace esp_idf { inline namespace v1 {

namespace detail {

// FIX: We may have to inspect priority_queue to get proper next (aka next after next)
// in which case templating is gonna be required.  (See PGESP-71 worries)
// It's conceivable we lock down item type a bit i.e. make a gptimer_scheduler_item
// as a virtual base.  This reduces the templating issue.  Since virtualization is already
// not off the table for v2 scheduling, this could be viable.  That said, we still have the issue
// of the container type itself.
// Perhaps it's worth it to force quasi-specialize to something like a layer1 20-slot container.
// DEBT: Consider using inline attribute to 100% ensure this gets inlined
inline bool gptimer_context::alarm_cb(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t* edata)
{
    BaseType_t awake {};
    // NOTE: My understanding is 100% of the time, this will set awake as true
    // since our service task is presumed high priority
    task_.notify_from_isr(0, eNoAction, &awake);
    return awake;
}

bool IRAM_ATTR gptimer_context::alarm_cb(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t* edata,
    void* user_ctx)
{
    return ((gptimer_context*)user_ctx)->alarm_cb(timer, edata);
}

}

}}}}

#endif
