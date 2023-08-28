#include <estd/port/freertos/queue.h>
#include <estd/port/freertos/ring_buffer.h>

#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/esp-now.h>
#include <embr/platform/esp-idf/service/event.h>


struct App
{
    static constexpr const char* TAG = "App";

    using EspNow = embr::esp_idf::service::v1::EspNow;

    void on_notify(EspNow::event::receive);
    void on_notify(EspNow::event::send);

    estd::freertos::wrapper::ring_buffer ring;

    App()
    {
        // DEBT: We can almost use a split ringbuffer here, excluding
        // the header
        ring.create(1024, RINGBUF_TYPE_NOSPLIT);
    }
};