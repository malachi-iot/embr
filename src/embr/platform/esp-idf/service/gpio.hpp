#include "gpio.h"

#include <embr/platform/esp-idf/log.h>

//#define ESP_INTR_FLAG_DEFAULT 0

//void init_gpio_ll();

namespace embr::esp_idf {

namespace service { inline namespace v1 {

// FIX: Need to resolve driver vs raw recommended way of getting
// triggered pin - and do we even reach here for multiple pins?  
// I think driver mode doesn't always return which pin actually activated it,
// because the ISR itself does activate but with a '0' gpio_intr_status
template <bool sparse>
template <class TSubject, class TImpl>
void GPIO<sparse>::runtime<TSubject, TImpl>::gpio_isr_handler()
{
    // Since we're using driver mode, no interrupt clear required

    uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);   //read status to get interrupt status for GPIO0-31
#if FEATURE_GPIOSERVICE_DRIVERMODE == 0
    // Fun fact - your ESP32 will reset if you don't clear your interrupts :)
    SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);    //Clear intr for gpio0-gpio31
#endif

    // Some devices have less than 32 GPIO
    // For example esp32-c3 has 22 or 16
#ifdef GPIO_STATUS1_REG
    uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);//read status1 to get interrupt status for GPIO32-39
#if FEATURE_GPIOSERVICE_DRIVERMODE == 0
    SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h); //Clear intr for gpio32-39
#endif
    notify(event::gpios{gpio_intr_status, gpio_intr_status_h});
#else
    notify(event::gpios{gpio_intr_status});
#endif

    //ESP_DRAM_LOGV(TAG, "gpio_isr_handler: entry %" PRIx32, gpio_intr_status);

    unsigned pin = 0;

    while(gpio_intr_status)
    {
        if(gpio_intr_status & 1)    notify(event::gpio{(gpio_num_t)pin});

        ++pin;
        gpio_intr_status >>= 1;
    }

#ifdef GPIO_STATUS1_REG
    pin = 32;

    while(gpio_intr_status_h)
    {
        if(gpio_intr_status_h & 1)    notify(event::gpio{(gpio_num_t)pin});

        ++pin;
        gpio_intr_status_h >>= 1;
    }
#endif
}


template <bool sparse>
template <class TSubject, class TImpl>
void GPIO<sparse>::runtime<TSubject, TImpl>::gpio_isr_handler(void* arg)
{
    ((runtime*) arg)->gpio_isr_handler();
}


template <bool sparse>
template <class TSubject, class TImpl>
embr::Service::state_result GPIO<sparse>::runtime<TSubject, TImpl>::on_start(
    const gpio_config_t* c, int intr_alloc_flags) //, embr::esp_idf::gpio pin)
{
    esp_err_t ret;

    // NOTE: Driver mode not fully functional, activated pin deduction
    // still required
#if FEATURE_GPIOSERVICE_DRIVERMODE

    ESP_ERROR_CHECK_WITHOUT_ABORT(ret =
        gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));

    if(ret != ESP_OK)
    {
        return state_result{Error, ErrConfig};
    }
#endif

    base_type::configuring(c);

    ESP_ERROR_CHECK_WITHOUT_ABORT(ret = gpio_config(c));

#if FEATURE_GPIOSERVICE_DRIVERMODE
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret =
        gpio_isr_handler_add(pin, gpio_isr_handler, this));
#else
    gpio_isr_handle_t* h = base_type::gpio_isr_handle();
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret =
            gpio_isr_register(gpio_isr_handler, this, intr_alloc_flags, h));
#endif

    if(ret == ESP_OK)
    {
        base_type::configured(c);
        return state_result::started();
    }

    return state_result{ServiceEnum::Error, ServiceEnum::ErrConfig};
}

}}

}
