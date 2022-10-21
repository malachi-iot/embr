#include "esp_log.h"

#include "timer-scheduler.hpp"

namespace embr { namespace esp_idf {

typedef embr::internal::layer1::Scheduler<5, ThresholdImpl> scheduler_type;

void timer_scheduler_init(embr::esp_idf::Timer& timer, uint32_t divider, timer_isr_t isr_handler, void* arg)
{
    const char* TAG = "timer_scheduler_init";

    ESP_LOGI(TAG, "group=%d, idx=%d, arg=%p", timer.group, timer.idx, arg);

    timer_config_t config;

    config.divider = divider; 

    config.counter_dir = TIMER_COUNT_UP;
    config.alarm_en = TIMER_ALARM_DIS;
    config.intr_type = TIMER_INTR_LEVEL;
    //config.auto_reload = TIMER_AUTORELOAD_DIS;
    config.auto_reload = TIMER_AUTORELOAD_EN; // Reset timer to 0 when end condition is triggered
    config.counter_en = TIMER_PAUSE;
#if SOC_TIMER_GROUP_SUPPORT_XTAL
    config.clk_src = TIMER_SRC_CLK_APB;
#endif
    timer.init(&config);

    timer.set_counter_value(0);
    timer.isr_callback_add(isr_handler, arg, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM);
    timer.enable_intr();
}

void timer_scheduler_tester()
{
    const char* TAG = "timer_scheduler_tester";

    embr::esp_idf::Timer timer(TIMER_GROUP_0, TIMER_1);

    static TimerScheduler<scheduler_type> ts(timer);

    ts.init();

    scheduler_type::value_type scheduled(estd::chrono::milliseconds(250));

    ts.schedule(scheduled);

    ESP_LOGD(TAG, "group=%d, idx=%d, scheduled=%u", ts.timer.group, ts.timer.idx,
        scheduled.wakeup.count());
}

}}

