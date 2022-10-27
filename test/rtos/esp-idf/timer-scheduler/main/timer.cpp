#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include <embr/platform/esp-idf/timer-scheduler.hpp>

using namespace embr::esp_idf;

typedef DurationImpl impl_type;
typedef embr::internal::layer1::Scheduler<5, impl_type> scheduler_type;

void timer_scheduler_tester()
{
    DurationImpl2<int, -1> test1;
    DurationImpl2<unsigned, 80> test2;

    auto v1 = test1.divisor();
    auto v2 = test2.divisor();

    const char* TAG = "timer_scheduler_tester";

    embr::esp_idf::Timer timer(TIMER_GROUP_0, TIMER_1);

    static scheduler_type scheduler(embr::internal::scheduler::impl_params_tag{}, timer);

    scheduler.init(&scheduler);

    {
    scheduler_type::value_type scheduled(estd::chrono::milliseconds(250));

    scheduler.schedule(scheduled);

    }

    {
    scheduler_type::value_type scheduled(estd::chrono::milliseconds(500));

    scheduler.schedule(scheduled);
    }
}


