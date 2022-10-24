#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include <embr/platform/esp-idf/timer-scheduler.hpp>

using namespace embr::esp_idf;

typedef embr::internal::layer1::Scheduler<5, DurationImpl> scheduler_type;

void timer_scheduler_tester()
{
    const char* TAG = "timer_scheduler_tester";

    embr::esp_idf::Timer timer(TIMER_GROUP_0, TIMER_1);

    static TimerScheduler<scheduler_type> ts(timer);

    ts.init();

    {
    scheduler_type::value_type scheduled(estd::chrono::milliseconds(250));

    ts.schedule(scheduled);

    ESP_LOGD(TAG, "group=%d, idx=%d, scheduled=%u", ts.timer.group, ts.timer.idx,
        scheduled.wakeup.count());
    }

    {
    scheduler_type::value_type scheduled(estd::chrono::milliseconds(500));

    ts.schedule(scheduled);

    ESP_LOGD(TAG, "group=%d, idx=%d, scheduled=%u", ts.timer.group, ts.timer.idx,
        scheduled.wakeup.count());
    }
}


