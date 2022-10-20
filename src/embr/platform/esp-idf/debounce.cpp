//#include <estd/map.h>

// DEBT: Don't want to do this dynamic alloc version, though not SO bad because it's the
// "alloc once" category
#include <map>

#include "debounce.hpp"

#include <driver/gpio.h>
#include <driver/timer.h>

namespace embr { inline namespace esp_idf {

// Guidance from:
// https://esp32.com/viewtopic.php?t=345 

// DEBT: upgrade estd map to use vector, since maps indeed should be dynamic even
// in our basic use cases
//static estd::layer1::map<uint8_t, detail::Debouncer, 5> debouncers;
static std::map<uint8_t, detail::Debouncer> debouncers;
static gpio_isr_handle_t gpio_isr_handle;
static timer_isr_handle_t timer_isr_handle;
static timer_group_t timer_group = TIMER_GROUP_0;

static void gpio_isr(void* context)
{
    uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);   //read status to get interrupt status for GPIO0-31
    uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);//read status1 to get interrupt status for GPIO32-39
    // Fun fact - your ESP32 will reset if you don't clear your interrupts :)
    SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);    //Clear intr for gpio0-gpio31
    SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h); //Clear intr for gpio32-39

    // https://www.esp32.com/viewtopic.php?t=20123 indicates we can call this here
    auto now = estd::chrono::esp_clock::now();

    int pin = 0;

    while(gpio_intr_status)
    {
        if(gpio_intr_status & 1)
        {
            auto it = debouncers.find(pin);

            if(it != std::end(debouncers))
            {
                embr::detail::Debouncer& d = it->second;
            }
        }

        gpio_intr_status >>= 1;
        ++pin;
    }
}

// Guidance from
// https://www.esp32.com/viewtopic.php?t=12931 
static void IRAM_ATTR timer_group0_isr (void *param)
{
    TIMERG0.int_clr_timers.t0 = 1; //clear interrupt bit

    // DEBT: This is an expensive call, and we can compute
    // the time pretty handily by inspecting our own timer instead
    // as per https://esp32.com/viewtopic.php?t=16228
    auto now = estd::chrono::esp_clock::now();
}

static void timer_init()
{
    timer_config_t timer;
    
    // Set prescaler for 10 KHz clock.  We'd go slower if we could, but:
    // "The divider’s range is from from 2 to 65536."
    timer.divider = 8000; 

    timer.counter_dir = TIMER_COUNT_UP;
    timer.alarm_en = TIMER_ALARM_DIS;
    timer.intr_type = TIMER_INTR_LEVEL;
    timer.auto_reload = TIMER_AUTORELOAD_EN; // Reset timer to 0 when end condition is triggered
    timer.counter_en = TIMER_PAUSE;

    timer_init(timer_group, TIMER_0, &timer);
    timer_set_counter_value(timer_group, TIMER_0, 0);
    timer_isr_register(timer_group, TIMER_0, timer_group0_isr, NULL, 
        ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
        &timer_isr_handle);
    
    // Brings us to an alarm every 10ms (100 counting / 10 Khz = 0.01s = 10ms)
    timer_set_alarm_value(timer_group, TIMER_0, 100);
    timer_start(timer_group, TIMER_0);

    timer_enable_intr(timer_group, TIMER_0);
}


void debouncer_init()
{
    ESP_ERROR_CHECK(
        gpio_isr_register(gpio_isr, NULL, ESP_INTR_FLAG_LEVEL1, &gpio_isr_handle));
    timer_init();
}

void debouncer_deinit()
{
    esp_intr_free(gpio_isr_handle);    

    timer_set_alarm(timer_group, TIMER_0, TIMER_ALARM_DIS);
    timer_disable_intr(timer_group, TIMER_0);
    esp_intr_free(timer_isr_handle);    
}


}}