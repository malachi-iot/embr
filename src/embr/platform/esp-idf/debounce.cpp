#include "features/debounce.h"

#if defined(ESP_PLATFORM) && FEATURE_EMBR_ESP_TIMER_SCHEDULER

// DEBT: Don't want to do this dynamic alloc version, though not SO bad because it's the
// "alloc once" category
#include <map>

#include "debounce.hpp"
#include "timer.h"
#include <estd/port/freertos/timer.h>

#include "log.h"

#include "timer-scheduler.hpp"

#include <soc/gpio_reg.h>

namespace embr { namespace esp_idf {


static const char* TAG = "Debouncer";


using embr::scheduler::esp_idf::impl::Item;

// Guidance from:
// https://esp32.com/viewtopic.php?t=345 

using namespace estd::chrono;
using namespace estd::chrono_literals;


void held_callback(TimerHandle_t);

#if CONFIG_TIMER_CRASH_DIAGNOSTIC
#warning Compiling in crash diagnostic mode
static estd::freertos::timer<> held_timer("held", 3s, true, nullptr, held_callback);
#else
static estd::freertos::timer<> held_timer("held", 3s, false, nullptr, held_callback);
#endif

// DEBT: Only 1 GPIO handled in long-held mode
static int gpio_long_held = -1;

inline void Debouncer::gpio_isr_pin(Item& item, esp_clock::duration duration)
{
    static constexpr const char* TAG = "gpio_isr_pin";

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
    //static constexpr const char* TAG = "gpio_isr";

    uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);   //read status to get interrupt status for GPIO0-31
    // Fun fact - your ESP32 will reset if you don't clear your interrupts :)
    SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);    //Clear intr for gpio0-gpio31

    // Some devices have less than 32 GPIO
    // For example esp32-c3 has 22 or 16
#ifdef GPIO_STATUS1_REG
    uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);//read status1 to get interrupt status for GPIO32-39
    SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h); //Clear intr for gpio32-39
#endif

    // https://www.esp32.com/viewtopic.php?t=20123 indicates we can call this here
    auto now = esp_clock::now();
    auto duration = now - last_now;

    int pin = 0;

    // DEBT: Upper gpio_intr_status_h not yet supported

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


void Debouncer::gpio_isr_pin(void* context)
{
    auto item = static_cast<Item*>(context);
    Debouncer* debouncer = item->parent_;

    auto duration = esp_clock::now() - debouncer->last_now;

    debouncer->gpio_isr_pin(*item, duration);
}


// these eval code blocks are disabled because I misunderstood that timer pend
// FreeRTOS calls are for scheduling immediate callbacks, not later one-shots

/*
static void end_long_hold_evaluation(void* pvParameter1, uint32_t ulParameter2)
{
    auto& item = *(Item*) pvParameter1;

    //ESP_LOGD(TAG, "end_long_hold_evaluation");

    item.long_hold_evaluating = false;
}
*/

// NOTE: Put IRAM_ATTR to help with strange timer related crash only.  It didn't help
static void IRAM_ATTR begin_long_hold_evaluation(void* pvParameter1, uint32_t ulParameter2)
{
    //static volatile bool mutex = false; // NOTE: Put fake-mutex in here to help with crash, still doesn't help

    //if(mutex) return;

    //mutex = true;

    auto& item = *(Item*) pvParameter1;
    //item.long_hold_evaluating = true;

    //ESP_GROUP_LOGD(1, TAG, "begin_long_hold_evaluation");
    ESP_DRAM_LOGV(TAG, "begin_long_hold_evaluation");

    //held_timer.id(pvParameter1);  // not safe from ISR - FIX: Need a workaround
    //held_timer.reset(100ms);    // DEBT: Don't want to do this from pend call, could cause a deadlock maybe?

    // FIX: Somehow this causes a catastrophic failure in FreeRTOS *and* LwIP
    // Specifically it dies within prvProcessReceivedCommands while calling uxListRemove
    // and crashes in or around lwip_cyclic_timer/uxListRemove - almost like a data race
    // while updating the 'pxItemToRemove' in uxListRemove.  Most likely cause of that is
    // my wrapper not managing the timer id/handle right
    
    // Plot thickens:
    // If pressed before held_timer expires the first time, it does not crash.
    // This suggests once expiry hits, reset may not be viable.  We don't auto delete the timer
    // estd testing indicates reset DOES work from inside the timer callback

    BaseType_t  pxHigherPriorityTaskWoken;
    //held_timer.native().start_from_isr(&pxHigherPriorityTaskWoken);   // EXPERIMENTING, doesn't help
    held_timer.reset_from_isr(&pxHigherPriorityTaskWoken);
    gpio_long_held = item.pin();

    // Also:
    // FreeRTOS timers are missing ISR-safe ways to interact with ID, which may become an important factor here
    
    /*
    estd::freertos::internal::timer::pend_function_call(
        end_long_hold_evaluation,
        pvParameter1,
        ulParameter2,
        10s);
        */

    //xTimerPendFunctionCall(pvParameter1, pvParameter2);

    //mutex = false;
}


void Debouncer::emit_state(const Item& item)
{
    bool on = item.debouncer().state();

    if(item.low_means_pressed) on = !on;

    if(on)
    {
        // EXPERIMENTAL
        if(item.long_hold_evaluating == false)
        {
            // DEBT: Retrieve higherpriority task part
            //xTimerPendFunctionCallFromISR(begin_long_hold_evaluation, (void*)&item, 0, nullptr);
            begin_long_hold_evaluation((void*)&item, 0);
        }
    }
    else
    {
        BaseType_t  pxHigherPriorityTaskWoken;
        held_timer.stop_from_isr(&pxHigherPriorityTaskWoken);
    }

    queue.send_from_isr(Notification{item.pin(), (States)on});
}



Debouncer::Debouncer(timer_group_t timer_group, timer_idx_t timer_idx, bool driver_mode) //: queue(10)
    : scheduler(embr::internal::scheduler::impl_params_tag{}, timer_group, timer_idx)
{
    ESP_LOGI(TAG, "ctor: driver_mode=%u", driver_mode);

    if(driver_mode)
    {
        gpio_isr_handle = nullptr;
    }
    else
    {
        ESP_ERROR_CHECK(
            gpio_isr_register(gpio_isr, this, ESP_INTR_FLAG_LEVEL1, &gpio_isr_handle));
    }

    scheduler.start();
    last_now = esp_clock::now();
    held_timer.id(this);    // DEBT: This will eventually be Item* if we continue to use FreeRTOS timers
    //held_timer.start(1s);
}

Debouncer::~Debouncer()
{
    if(is_driver_mode())
    {
        // NOTE: Consider putting this into destructor.  Somehow feels more natural here though
        for(std::pair<const uint8_t, Item>& item : debouncers)
        {
            ESP_ERROR_CHECK(gpio_isr_handler_remove(item.second.pin()));
        }
    }
    else
        esp_intr_free(gpio_isr_handle);    
}

void Debouncer::track(int pin)
{
    //debouncers.insert_or_assign(pin, embr::detail::Debouncer{});
    // FIX: Clearly not a great way to do things, but I do not yet understand how std::map
    // regular 'insert' and 'emplace' work
    //Item item(this, (gpio_num_t)pin);
    //debouncers[pin] = item;
    
    // DEBT: Avoid auto here
    auto emplaced = debouncers.emplace(std::make_pair(
        pin,
        Item(this, (gpio_num_t)pin)));

    if(emplaced.second)
    {
        // DEBT: This magic will be unveiled when above avoiding auto debt is fulfilled
        Item& item = emplaced.first->second;

        if(is_driver_mode())
        {
            ESP_ERROR_CHECK(gpio_isr_handler_add(item.pin(), gpio_isr_pin, &item));
        }
    }
    else
        ESP_LOGE(TAG, "track: failed to emplace gpio tracking");
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

    // NOTE: This highly nasty shenanigan somehow corrupts memory.  Keeping here as a comment
    // for further study.  Consumed hours of my life.
    //auto timer = (estd::freertos::timer<>&) xTimer; 

    estd::freertos::internal::timer timer(xTimer);
    auto debouncer = (Debouncer*) timer.id();
    //auto item = (Item*) timer.id();

    ESP_LOGI(TAG, "Callback name: \"%s\", id=%p", timer.name(), debouncer);

    debouncer->queue.send(Debouncer::Notification{gpio_long_held, Debouncer::Held}, 50ms);
}


}}

#endif
