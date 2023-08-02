#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/gpio.h>


struct App
{
    static constexpr const char* TAG = "App";

    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    using GPIO = embr::esp_idf::service::v1::GPIO;

    void on_notify(GPIO::event::gpio);
};