#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/gpio.h>


struct App
{
    static constexpr const char* TAG = "App";

    // DEBT: Doesn't work I think we need to use uninitialized_array for layer1
    //estd::freertos::layer1::queue<embr::esp_idf::gpio, 5> q;
    estd::freertos::layer1::queue<gpio_num_t, 5> q;

    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    using GPIO = embr::esp_idf::service::v1::GPIO<false>;

    void on_notify(GPIO::event::gpio);
};