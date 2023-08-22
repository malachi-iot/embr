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
};