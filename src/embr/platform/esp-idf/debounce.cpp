//#include <estd/map.h>

// DEBT: Don't want to do this dynamic alloc version, though not SO bad because it's the
// "alloc once" category
#include <map>

#include "debounce.hpp"
#include "queue.h"
#include "timer.h"
#include "freertos/timer.h"

#include <esp_log.h>

#include "timer-scheduler.hpp"

namespace embr { namespace esp_idf {


using internal::Item;

// Guidance from:
// https://esp32.com/viewtopic.php?t=345 

// DEBT: Place these inside 'Debouncer'
// DEBT: upgrade estd map to use vector, since maps indeed should be dynamic even
// in our basic use cases
//static estd::layer1::map<uint8_t, detail::Debouncer, 5> debouncers;
static std::map<uint8_t, Item> debouncers;

using namespace estd::chrono;
using namespace estd::chrono_literals;

static esp_clock::time_point last_now;

void held_callback(TimerHandle_t);

embr::freertos::timer<> held_timer("held", 3s, false, nullptr, held_callback);


inline void Debouncer::gpio_isr()
{
    static constexpr const char* TAG = "gpio_isr";

    uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);   //read status to get interrupt status for GPIO0-31
    uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);//read status1 to get interrupt status for GPIO32-39
    // Fun fact - your ESP32 will reset if you don't clear your interrupts :)
    SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);    //Clear intr for gpio0-gpio31
    SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h); //Clear intr for gpio32-39

    // https://www.esp32.com/viewtopic.php?t=20123 indicates we can call this here
    auto now = esp_clock::now();
    auto duration = now - last_now;

    int pin = 0;

    while(gpio_intr_status)
    {
        if(gpio_intr_status & 1)
        {
            // DEBT: Would be better to do like esp32-button does it, track the pin
            // as part of a debouncer vector - iterate through those and reverse check
            // to see if we are masked to care
            auto it = debouncers.find(pin);

            int level = gpio_get_level((gpio_num_t)pin);

            ESP_DRAM_LOGD(TAG, "GPIO%d, val: %d, duration=%lldms", pin, level,
                duration.count() / 1000);

            if(it != std::end(debouncers))
            {
                Item& item = it->second;
                embr::detail::Debouncer& d = item.debouncer();
                ESP_DRAM_LOGD(TAG, "state=%s:%s", to_string(d.state()), to_string(d.substate()));
                bool state_changed = d.time_passed(duration, level);
                if(state_changed)
                {
                    ets_printf("2 Intr debounce state changed\n");
                    emit_state(item);

                    // If there was a timer waiting for state change, disable it
                    //timer_set_alarm(timer_group, timer_idx, TIMER_ALARM_DIS);
                    //timer_group_disable_alarm_in_isr(timer_group, timer_idx);
                }
                else if(d.substate() != embr::detail::Debouncer::Idle)
                {
                    //auto future = now + estd::chrono::milliseconds(40);
                    //timer_set_alarm_value(timer_group, timer_idx, future.time_since_epoch().count());
                    //timer_set_alarm_value(timer_group, timer_idx, 40000);
                    //timer_set_alarm(timer_group, timer_idx, TIMER_ALARM_EN);
                    //timer_group_set_alarm_value_in_isr(timer_group, timer_idx, 40000);

                    // NOTE: Not sure if we can/should do this in an ISR
                    //timer_set_counter_value(timer_group, timer_idx, 0);   // This works -- now trying pause method
                    
                    auto context = scheduler.create_context(true);  // isr context
                    // call isr-specific now()
                    item.wakeup_ = scheduler.now(true) + d.signal_threshold();
                    // DEBT: Sloppy assignment of last_wakeup_
                    item.last_wakeup_ = item.wakeup_;
                    
                    scheduler.schedule_with_context(context, &item);

                    // DEBT: Wrap the following up inside scheduler impl on_xxx itself, and
                    // set actual alarm_value also
                    Timer& timer = scheduler.timer();
                    timer.enable_alarm_in_isr();
                    timer.start();

                }
                ESP_DRAM_LOGD(TAG, "state=%s:%s", to_string(d.state()), to_string(d.substate()));
            }
        }

        gpio_intr_status >>= 1;
        ++pin;
    }

    last_now = now;
}

void Debouncer::gpio_isr(void* context)
{
    static_cast<Debouncer*>(context)->gpio_isr();
}

#if UNUSED
inline void IRAM_ATTR Debouncer::timer_group0_isr()
{
    uint64_t counter;
    
    //timer_get_counter_value(timer_group, timer_idx, &counter);
    //counter = timer_group_get_counter_value_in_isr(timer_group, timer_idx);
    counter = timer.get_counter_value_in_isr();

    // DEBT: This is an expensive call, and we can compute
    // the time pretty handily by inspecting our own timer instead
    // as per https://esp32.com/viewtopic.php?t=16228
    auto now = estd::chrono::esp_clock::now();
    auto duration = now - last_now;

    // Since GPIO ISR was presumably not called yet, state hasn't changed
    //bool level = d.state();
    bool level = gpio_get_level((gpio_num_t)0);

    ets_printf("1 Timer Intr, duration=%lldus, timer_counter=%lld, level=%d\n",
        duration.count(), counter, level);
        
    auto& item = debouncers[0];
    auto& d = item.debouncer();
    ets_printf("2 Timer Intr state=%s:%s\n", to_string(d.state()), to_string(d.substate()));
    bool state_changed = d.time_passed(duration, level);
    ets_printf("3 Timer Intr state=%s:%s\n", to_string(d.state()), to_string(d.substate()));

    if(state_changed)
    {
        ets_printf("3.1 Timer Intr debounce state changed\n");
        emit_state(item);
    }

    last_now = now;
}
#endif

void Debouncer::emit_state(const Item& item)
{
    bool on = item.debouncer().state();

    if(item.low_means_pressed) on = !on;

    queue.send_from_isr(Notification{item.pin(), (States)on});
}

#if UNUSED
// Guidance from
// https://www.esp32.com/viewtopic.php?t=12931 
void IRAM_ATTR Debouncer::timer_group0_isr (void *param)
{
    // NOTE: Last I checked, this flavor doesn't work quite right.

    TIMERG0.int_clr_timers.t0 = 1; //clear interrupt bit

    static_cast<Debouncer*>(param)->timer_group0_isr();
}


bool IRAM_ATTR Debouncer::timer_group0_callback (void *param)
{
    timer.pause();

    static_cast<Debouncer*>(param)->timer_group0_isr();

    // DEBT: Pretty sure we need an ISR-friendly version of this.  However, I can't find one
    //timer_set_alarm(timer_group, timer_idx, TIMER_ALARM_DIS);
    // Above not needed, becase
    // "Once triggered, the alarm is disabled automatically and needs to be re-enabled to trigger again."

    return false;
}

void Debouncer::timer_init(bool callback_mode)
{
    const char* TAG = "Debouncer::timer_init";

    embr::esp_idf::Timer& timer_alias = timer;

    {
    timer_config_t timer;
    
    // Set prescaler for 1 MHz clock - remember, we're dividing
    // "default is APB_CLK running at 80 MHz"
    timer.divider = 80; 

    timer.counter_dir = TIMER_COUNT_UP;
    timer.alarm_en = TIMER_ALARM_DIS;
    timer.intr_type = TIMER_INTR_LEVEL;
    //timer.auto_reload = TIMER_AUTORELOAD_DIS;
    timer.auto_reload = TIMER_AUTORELOAD_EN; // Reset timer to 0 when end condition is triggered
    timer.counter_en = TIMER_PAUSE;
#if SOC_TIMER_GROUP_SUPPORT_XTAL
    timer.clk_src = TIMER_SRC_CLK_APB;
#endif
    timer_alias.init(&timer);
    }

    timer.set_counter_value(0);

    if(callback_mode == false)
    {
        ESP_LOGD(TAG, "ISR low level mode");
        timer_isr_register(timer.group, timer.idx, timer_group0_isr, this, 
            ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
            &timer_isr_handle);
    }
    else
    {
        ESP_LOGD(TAG, "ISR callback mode");
        timer_isr_callback_add(timer.group, timer.idx, timer_group0_callback, this,
            ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM);
    }
    
    // Brings us to an alarm at 41ms when enabled.  This is to give us 1ms wiggle room
    // when detecting debounce threshold of 40ms
    // DEBT: All that needs to be configurable
    timer.set_alarm_value(41000);
    //timer_start(timer_group, timer_idx);

    timer.enable_intr();
}
#endif


Debouncer::Debouncer(timer_group_t timer_group, timer_idx_t timer_idx) //: queue(10)
    : scheduler(embr::internal::scheduler::impl_params_tag{}, timer_group, timer_idx)
{
    ESP_ERROR_CHECK(
        gpio_isr_register(gpio_isr, this, ESP_INTR_FLAG_LEVEL1, &gpio_isr_handle));
    scheduler.init();
    last_now = esp_clock::now();
    held_timer.start(1s);
}

Debouncer::~Debouncer()
{
    esp_intr_free(gpio_isr_handle);    

#if UNUSED
    timer.set_alarm(TIMER_ALARM_DIS);
    timer.disable_intr();

#if CONFIG_ISR_LOW_LEVEL_MODE
#elif CONFIG_ISR_CALLBACK_MODE
    timer_isr_callback_remove(timer.group, timer.idx);
#endif

    esp_intr_free(timer_isr_handle);    
#endif
}

void Debouncer::track(int pin)
{
    //debouncers.insert_or_assign(pin, embr::detail::Debouncer{});
    // FIX: Clearly not a great way to do things, but I do not yet understand how std::map
    // regular 'insert' and 'emplace' work
    //Item item(this, (gpio_num_t)pin);
    //debouncers[pin] = item;
    debouncers.emplace(std::make_pair(
        pin,
        Item(this, (gpio_num_t)pin)));
}


const char* to_string(Debouncer::States state)
{
    switch(state)
    {
        case Debouncer::Up:     return "Up";
        case Debouncer::Down:   return "Down";
        case Debouncer::Held:   return "Held";
        default:                return "N/A";
    }
}

void held_callback(TimerHandle_t xTimer)
{
    const char* TAG = "held_callback";

    auto timer = (embr::freertos::timer<>&) xTimer; 

    ESP_LOGI(TAG, "Callback name: \"%s\"", timer.name());
}


}}