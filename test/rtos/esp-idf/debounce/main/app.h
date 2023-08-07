#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/gptimer.h>
#include <embr/platform/esp-idf/service/pm.h>

struct App
{
    static constexpr const char* TAG = "App";

    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    using Timer = embr::esp_idf::service::v1::GPTimer;

    void on_notify(Timer::event::callback);
};