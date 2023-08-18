#include <estd/port/freertos/queue.h>

#include <embr/service.h>

#include <embr/platform/esp-idf/debounce2.h>
#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/event.h>
#include <embr/platform/esp-idf/service/gptimer.h>


struct App
{
    static constexpr const char* TAG = "App";

    using Event = embr::debounce::v1::Event;
    using Timer = embr::esp_idf::service::v1::GPTimer;

    estd::freertos::layer1::queue<Event, 5> q;

    void on_notify(Timer::event::callback);

    // By way of embr::esp_idf::service::Event
    void on_notify(Event);
};


ESP_EVENT_DECLARE_BASE(DEBOUNCE_EVENT);


namespace embr::esp_idf::event {

inline namespace v1 { namespace internal {

template <>
struct handler<DEBOUNCE_EVENT>
{
    template <class Service>
    static constexpr bool exec(Service* s, int32_t id, void* data)
    {
        return ( s->notify(*(embr::debounce::v1::Event*) data), false );
    }
};

}}

}