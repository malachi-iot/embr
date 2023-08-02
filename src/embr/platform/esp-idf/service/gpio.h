#pragma once

#include <embr/platform/esp-idf/gpio.h>
#include <embr/service.h>

namespace embr::esp_idf {

namespace service { inline namespace v1 {

#define FEATURE_GPIOSERVICE_DRIVERMODE 0

#if FEATURE_GPIOSERVICE_DRIVERMODE
#warn GPIO service driver mode not fully functional
#endif

// This is specifically for listening for incoming GPIO on an isr
struct GPIO : embr::Service
{
    typedef GPIO this_type;

    static constexpr const char* TAG = "sd::gpio";
    static constexpr const char* name() { return TAG; }

    void init() {}

    gpio_isr_handle_t gpio_isr_handle;

    // These events are fired in ISR context
    struct event
    {
        // raw rebroadcast of gpio change state from ISR
        struct gpios
        {
            const uint32_t reg;
//#ifdef GPIO_STATUS1_REG
            const uint32_t reg_h;
//#endif
        };
        
        // which gpio in particular has changed, fired in rapid succession
        using gpio = embr::esp_idf::gpio;
    };

    //EMBR_PROPERTY_RUNTIME_BEGIN(embr::Service)

    //EMBR_PROPERTY_RUNTIME_END

    // TODO: Use helper macro here
    //EMBR_SERVICE_RUNTIME_BEGIN

    template <class TSubject = embr::void_subject, class TImpl = this_type>
    struct runtime : embr::Service::runtime<TSubject, TImpl>
    {
        typedef embr::Service::runtime<TSubject, TImpl> base_type;
        //using base_type::fire_changed;
        using base_type::notify;

        void IRAM_ATTR gpio_isr_handler();
        static void IRAM_ATTR gpio_isr_handler(void* arg);
        
        ESTD_CPP_FORWARDING_CTOR(runtime)

#if FEATURE_GPIOSERVICE_DRIVERMODE
        state_result on_start(const gpio_config_t*, embr::esp_idf::gpio pin);
#else
        // Defaults to level1 (lowest priority) interrupt
        state_result on_start(const gpio_config_t*,
            int intr_alloc_flags = ESP_INTR_FLAG_LEVEL1);
#endif
    };

    template <class TSubject>
    using static_type = static_factory<TSubject, this_type>::static_type;
};


struct GPIOSmoother : embr::Service
{
    typedef GPIOSmoother this_type;

    uint32_t levels = 0;

    static constexpr const char* TAG = "sd::gpio_smoother";
    static constexpr const char* name() { return TAG; }

    EMBR_PROPERTY_RUNTIME_BEGIN(embr::Service)

    EMBR_PROPERTY_RUNTIME_END
};



}}

}