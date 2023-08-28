#include <estd/port/freertos/queue.h>
#include <estd/port/freertos/ring_buffer.h>

#include <estd/internal/variant/storage.h>  // DEBT: In meantime until layer1::queue is improved...

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/esp-now.h>
#include <embr/platform/esp-idf/service/event.h>

// TODO: Put this into estd proper
namespace estd { namespace freertos { namespace layer1 {

// FIX: Not a true 'unwrapper' until we redo methods with chrono signatures
// TODO: It occurs to me that the "default" signature should be portMAX while
// the others can go all the way down to 0 easily.  This sidesteps the "how do I 
// represent portMAX with chrono" conundrum
template <size_t sz>
struct ring_buffer : estd::freertos::wrapper::ring_buffer
{
    uint8_t storage[sz];
    StaticRingbuffer_t native_;

    ring_buffer(RingbufferType_t type)
    {
        create(sz, type, storage, &native_);
    }


    template <class T, class ...Args>
    BaseType_t emplace_add_size(TickType_t xTicksToWait, size_t past_t_sz, Args&&...args)
    {
        T* t;

        if(send_acquire((void**)&t, sizeof(T) + past_t_sz, xTicksToWait) == pdFALSE)
            return pdFALSE;

        new (t) T(std::forward<Args>(args)...);

        return send_complete(t);
    }
};

}}}


struct App
{
    static constexpr const char* TAG = "App";

    using EspNow = embr::esp_idf::service::v1::EspNow;

    void on_notify(EspNow::event::receive);
    void on_notify(EspNow::event::send);

    estd::freertos::layer1::ring_buffer<1024> ring;

    // DEBT: We can almost use a split ringbuffer here, excluding
    // the header
    App() : ring(RINGBUF_TYPE_NOSPLIT)
    {
    }
};