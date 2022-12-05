//#include <estd/map.h>

// DEBT: Don't want to do this dynamic alloc version, though not SO bad because it's the
// "alloc once" category
#include <map>
#include <unordered_map>

#include "debounce.hpp"
#include "timer.h"
#include <estd/port/freertos/timer.h>

#include "log.h"

#include "timer-scheduler.hpp"

namespace embr { namespace esp_idf {


using embr::scheduler::esp_idf::impl::Item;

// Guidance from:
// https://esp32.com/viewtopic.php?t=345 

// DEBT: Place these inside 'Debouncer'
// DEBT: upgrade estd map to use vector, since maps indeed should be dynamic even
// in our basic use cases
//static estd::layer1::map<uint8_t, detail::Debouncer, 5> debouncers;
static std::unordered_map<uint8_t, Item> debouncers;

using namespace estd::chrono;
using namespace estd::chrono_literals;

static esp_clock::time_point last_now;

void held_callback(TimerHandle_t);

estd::freertos::timer<> held_timer("held", 3s, false, nullptr, held_callback);

inline void Debouncer::gpio_isr_pin(Item& item, esp_clock::duration duration)
{
    static constexpr const char* TAG = "gpio_isr";

    int level = item.pin().level();

    ESP_GROUP_LOGD(1, TAG, "GPIO%d, val: %d, duration=%lldms", (int)item.pin(), level,
        duration.count() / 1000);

    embr::detail::Debouncer& d = item.debouncer();
    ESP_GROUP_LOGD(1, TAG, "state=%s:%s", to_string(d.state()), to_string(d.substate()));
    bool state_changed = d.time_passed(duration, level);
    if(state_changed)
    {
        ESP_GROUP_LOGV(1, TAG, "debounce state changed");
        emit_state(item);

        // If there was a timer waiting for state change, disable it
        //timer_set_alarm(timer_group, timer_idx, TIMER_ALARM_DIS);
        //timer_group_disable_alarm_in_isr(timer_group, timer_idx);
    }
    else if(d.substate() != embr::detail::Debouncer::Idle)
    {
        // If no state change, but we are in eval mode, schedule a timer wakeup
        // to finish the eval

        //auto future = now + estd::chrono::milliseconds(40);
        //timer_set_alarm_value(timer_group, timer_idx, future.time_since_epoch().count());
        //timer_set_alarm_value(timer_group, timer_idx, 40000);
        //timer_set_alarm(timer_group, timer_idx, TIMER_ALARM_EN);
        //timer_group_set_alarm_value_in_isr(timer_group, timer_idx, 40000);

        // NOTE: Not sure if we can/should do this in an ISR
        //timer_set_counter_value(timer_group, timer_idx, 0);   // This works -- now trying pause method
        
        uint64_t native = scheduler.timer().get_counter_value_in_isr();
        // call isr-specific now()
        auto scheduler_now = scheduler.now(true);
        ESP_GROUP_LOGV(1, TAG, "now=%llu, raw=%llu", scheduler_now.count(), native);

        auto context = scheduler.create_context(true);  // isr context

        item.recalculate_event_due(scheduler_now);
        
        scheduler.schedule_with_context(context, &item);

        // DEBT: Wrap the following up inside scheduler impl on_xxx itself, and
        // set actual alarm_value also
        Timer& timer = scheduler.timer();
        timer.enable_alarm_in_isr();
        timer.start();

    }
    ESP_GROUP_LOGD(1, TAG, "state=%s:%s", to_string(d.state()), to_string(d.substate()));
}


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

            if(it != std::end(debouncers))
            {
                Item& item = it->second;
                gpio_isr_pin(item, duration);
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


void Debouncer::emit_state(const Item& item)
{
    bool on = item.debouncer().state();

    if(item.low_means_pressed) on = !on;

    queue.send_from_isr(Notification{item.pin(), (States)on});
}



Debouncer::Debouncer(timer_group_t timer_group, timer_idx_t timer_idx) //: queue(10)
    : scheduler(embr::internal::scheduler::impl_params_tag{}, timer_group, timer_idx)
{
    ESP_ERROR_CHECK(
        gpio_isr_register(gpio_isr, this, ESP_INTR_FLAG_LEVEL1, &gpio_isr_handle));
    scheduler.start();
    last_now = esp_clock::now();
    held_timer.start(1s);
}

Debouncer::~Debouncer()
{
    esp_intr_free(gpio_isr_handle);    
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

    auto timer = (estd::freertos::timer<>&) xTimer; 

    ESP_LOGI(TAG, "Callback name: \"%s\"", timer.name());
}


}}