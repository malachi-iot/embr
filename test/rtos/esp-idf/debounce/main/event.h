#pragma once

#include <embr/platform/esp-idf/service/event.h>

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
