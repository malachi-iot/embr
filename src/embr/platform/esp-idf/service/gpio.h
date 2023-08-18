#pragma once

#include <embr/platform/esp-idf/gpio.h>
#include <embr/service.h>

namespace embr::esp_idf {

namespace service { inline namespace v1 {

#define FEATURE_GPIOSERVICE_DRIVERMODE 0

#if FEATURE_GPIOSERVICE_DRIVERMODE
#warn GPIO service driver mode not fully functional
#endif

namespace internal {

template <bool sparse>
struct GPIO;


struct GPIOBase :
    embr::service::v1::ServiceBase//,         // for state_result
    //embr::property::v1::PropertyContainer   // for static_factory
{
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
};


template <>
struct GPIO<true> : embr::SparseService
{
    constexpr gpio_isr_handle_t* gpio_isr_handle() { return nullptr; }
};

template <>
struct GPIO<false> : embr::Service
{
    gpio_isr_handle_t gpio_isr_handle_;

#if __cpp_constexpr >= 201304L
    constexpr
#endif
    gpio_isr_handle_t* gpio_isr_handle() { return &gpio_isr_handle_; }
};

}

// This is specifically for listening for incoming GPIO on an isr
template <bool sparse>
struct GPIO : internal::GPIO<sparse> //, internal::GPIOBase
{
    typedef internal::GPIO<sparse> base_type;
    typedef GPIO<sparse> this_type;

    //using typename base_type::state_result;
    using state_result = embr::Service::state_result;
    using event = internal::GPIOBase::event;

    static constexpr const char* TAG = "GPIO";
    static constexpr const char* name() { return TAG; }

    void init() {}

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
    //using static_type = static_factory<TSubject, this_type>::static_type;
    using static_type = typename PropertyContainer::static_factory<TSubject, this_type>::static_type;
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