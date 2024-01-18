#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/adc.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/gpio.h>


class App
{
    static constexpr const char* TAG = "App";

public:
    struct gpio
    {
        gpio_num_t pin : 7;
        unsigned level : 1;
    };

    struct io
    {
        uint16_t value;
    };

    // DEBT: Make this private
    estd::freertos::layer1::queue<gpio, 5> q;
    estd::freertos::layer1::queue<io, 5> q2;

private:
    // DEBT: No doubt this is clumsy.  Referring to app singleton
    // from within a static void
    static void start(const adc_continuous_handle_cfg_t*,
        const adc_continuous_config_t*);

    void start();

public:
    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    using GPIO = embr::esp_idf::service::v1::GPIO<false>;
    using ADC = embr::esp_idf::service::v1::ADC;

    void on_notify(GPIO::event::gpio);
    void on_notify(ADC::event::converted);
};


namespace app_domain {

extern App app;

using singleton = estd::integral_constant<App*, &app>;
using top_tier = embr::layer0::subject<singleton>;

extern App::ADC::runtime<top_tier> adc;

}
