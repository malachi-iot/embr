#include <estd/port/freertos/queue.h>
#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/event.h>
#include <embr/platform/esp-idf/service/pm.h>
#include <embr/platform/esp-idf/service/protocomm.h>


struct App
{
    static constexpr const char* TAG = "App";

    static constexpr const char ep3[] = "test3";

    void do_notify(embr::esp_idf::service::v1::Protocomm::event::tag<int>);
    void do_notify(embr::esp_idf::service::v1::Protocomm::event::request_named<ep3>);
};