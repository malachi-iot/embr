#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include <embr/platform/esp-idf/timer-scheduler.hpp>

using namespace embr::esp_idf;

typedef DurationImpl impl_type;
typedef embr::internal::layer1::Scheduler<5, impl_type> scheduler_type;
static constexpr embr::internal::scheduler::impl_params_tag params_tag;

struct control_structure
{
    typedef long time_point;
};

struct control_structure1
{
    typedef estd::chrono::milliseconds time_point;

    time_point wakeup_;

    const time_point& event_due() const { return wakeup_; }

    bool process(time_point current_time) { return false; }
};

void timer_scheduler_tester()
{
    constexpr embr::esp_idf::Timer timer(TIMER_GROUP_0, TIMER_1);

    DurationImpl2<control_structure, -1, int> test1(timer);
    DurationImpl2<control_structure, 80, unsigned> test2(timer);
    DurationImpl2<control_structure1*, 80> test3(timer);

    auto v1 = test1.numerator();
    auto v2 = test2.numerator();
    //auto v3 = test3.now();    // "works" but crashes since timer isn't yet initialized
    //auto v3_count = v3.count();

    static embr::internal::layer1::Scheduler<5, decltype(test3)> s3(params_tag, TIMER_GROUP_0, TIMER_0);

    s3.init(&s3);

    const char* TAG = "timer_scheduler_tester";

    static scheduler_type scheduler(params_tag, timer);

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


