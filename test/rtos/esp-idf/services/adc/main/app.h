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
    using ADC = embr::esp_idf::service::v1::ADC;

    struct io
    {
        using pointer = ADC::event::converted::pointer;

        pointer begin, end;
    };

    // DEBT: Make this private
    estd::freertos::layer1::queue<io, 5> q;

private:
    // DEBT: No doubt this is clumsy.  Referring to app singleton
    // from within a static void
    static void start(const adc_continuous_handle_cfg_t*,
        const adc_continuous_config_t*);

public:
    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    void on_notify(ADC::event::converted);

    void start();
};


namespace app_domain {

extern App app;

using singleton = estd::integral_constant<App*, &app>;
using top_tier = embr::layer0::subject<singleton>;

extern App::ADC::runtime<top_tier> adc;

}
