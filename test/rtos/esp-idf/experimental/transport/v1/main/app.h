#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/twai.h>

struct App
{
    static constexpr const char* TAG = "App";

    template <class T>
    using changed = embr::event::PropertyChanged<T>;

    using TWAI = embr::esp_idf::service::v1::TWAI;

    void on_notify(TWAI::event::alert);
};