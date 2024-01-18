#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/adc.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/gpio.h>


struct App
{
    static constexpr const char* TAG = "App";

    struct gpio
    {
        gpio_num_t pin : 7;
        unsigned level : 1;
    };

    // DEBT: Doesn't work I think we need to use uninitialized_array for layer1
    //estd::freertos::layer1::queue<embr::esp_idf::gpio, 5> q;
    estd::freertos::layer1::queue<gpio, 5> q;

    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    using GPIO = embr::esp_idf::service::v1::GPIO<false>;
    using ADC = embr::esp_idf::service::v1::ADC;

    void on_notify(GPIO::event::gpio);
    void on_notify(ADC::event::converted);
};


namespace app_domain {

extern App app;

}
