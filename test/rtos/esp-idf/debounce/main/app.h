#include <estd/port/freertos/queue.h>

#include <embr/service.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/gptimer.h>
#include <embr/platform/esp-idf/service/pm.h>


enum DebounceEnum
{
    BUTTON_UNDEFINED,
    BUTTON_PRESSED,
    BUTTON_RELEASED
};


// DEBT: Can't do const here because layer1::queue's array doesn't
// play nice with it.  Upgrade layer1::queue to use 'uninitialized_array' and
// filter by is_trivial, is_trivially_copyable or is_trvially_copy_assignable
struct Item
{
    DebounceEnum state : 2;
    unsigned pin : 6;
};



struct App
{
    static constexpr const char* TAG = "App";

    estd::freertos::layer1::queue<Item, 10> q;

    using Timer = embr::esp_idf::service::v1::GPTimer;

    void on_notify(Timer::event::callback);
};