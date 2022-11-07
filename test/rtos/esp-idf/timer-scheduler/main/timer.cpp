#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include <embr/platform/esp-idf/timer-scheduler.hpp>

using namespace embr::esp_idf;
using namespace estd::chrono_literals;

typedef embr::internal::scheduler::impl::ReferenceBase<
    estd::chrono::duration<uint32_t, estd::milli> >::value_type control_structure;

typedef DurationImpl2<control_structure> impl_type;
typedef embr::internal::layer1::Scheduler<5, impl_type> scheduler_type;
static constexpr embr::internal::scheduler::impl_params_tag params_tag;

template <class TTimePoint>
struct control_structure_base : 
    embr::internal::scheduler::impl::ReferenceBase<TTimePoint>::value_type
{
    typedef typename embr::internal::scheduler::impl::ReferenceBase<TTimePoint>::value_type base_type;
    using typename base_type::time_point;

    static constexpr const char* TAG = "control_structure1";

    constexpr control_structure_base() = default;
    constexpr control_structure_base(time_point t) : base_type(t) {}

    bool process(time_point current_time)
    {
        // This variant of log macro works inside ISRs
        ESP_DRAM_LOGI(TAG, "process: wake=%llu, current=%llu",
            base_type::event_due_.count(),
            current_time.count());

        base_type::event_due_ += 1s;

        return true;    // Signal we want a reschedule
    }
};

typedef control_structure_base<estd::chrono::duration<int8_t, estd::ratio<1, 10> > > control_structure2;
typedef control_structure_base<estd::chrono::milliseconds> control_structure1;

void timer_scheduler_tester()
{
    constexpr embr::esp_idf::Timer timer(TIMER_GROUP_0, TIMER_1);

    // FIX: time_point::max() and similar not fleshed out yet - explicit time_point specified here
    // must match up with control_structure
    // TODO: Along with above, we have the notion of a 'native' time base for scheduler - for those
    // who don't want to do all the divisor conversions
    DurationImpl2<control_structure, -1, int> test1(timer);

    DurationImpl2<control_structure2, 80> test2(timer);
    DurationImpl2<control_structure1*, 80> test3(timer);

    auto v1 = test1.numerator();
    auto v2 = test2.numerator();
    //auto v3 = test3.now();    // "works" but crashes since timer isn't yet initialized
    //auto v3_count = v3.count();

    static embr::internal::layer1::Scheduler<5, decltype(test2)> s2(params_tag, TIMER_GROUP_1, TIMER_0);
    static embr::internal::layer1::Scheduler<5, decltype(test3)> s3(params_tag, TIMER_GROUP_0, TIMER_0);
    static control_structure1 c1{1s};

    s2.start();
    s2.schedule(50ms);  // For overflow testing

    s3.start();
    // DEBT: Something happening here generating create_context twice - things still work
    // and I have a feeling it's RVO related in which case it's harmless, but it bears investigation
    s3.schedule(&c1);

    const char* TAG = "timer_scheduler_tester";

    static scheduler_type scheduler(params_tag, timer);

    scheduler.start(16000);

    scheduler.schedule(estd::chrono::milliseconds(250));
    scheduler.schedule(estd::chrono::milliseconds(500));
}


