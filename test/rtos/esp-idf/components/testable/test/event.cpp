#include <esp_log.h>

#include <unity.h>

#include <embr/platform/esp-idf/event/v1/event.h>

using namespace embr;

static const char* TAG = "embr::unity::event";

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

enum SYNTHETIC_EVENTS2
{
    SYNTHETIC2_EVENT_1,
    SYNTHETIC2_EVENT_2
};


EMBR_ESP_EVENT_DECLARE_BASE(SYNTHETIC_EVENTS2);

//static constexpr esp_event_base_t TEST2 = SYNTHETIC_EVENTS2;

EMBR_IDF_EVENT_TRAITS(SYNTHETIC_EVENTS2, SYNTHETIC2_EVENT_1, int);



TEST_CASE("typed event", "[event]")
{
    constexpr esp_event_loop_args_t loop_args
    {
        .queue_size = 5,
    };

    // DEBT: Make our own event loop here, not the system one.  Race condition awaits
    // us since there's no gauruntee the post chain finishes in time
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    static int counter = 0;
    int counter2 = 10;

    ESP_LOGV(TAG, "GOT HERE: %s", SYNTHETIC_EVENTS2);

    auto f = [](int* data)
    {
        counter += *data;
    };
    esp_idf::event::handler_register_exp<SYNTHETIC_EVENTS2, SYNTHETIC2_EVENT_1>(f);

    esp_idf::event::post<SYNTHETIC_EVENTS2, SYNTHETIC2_EVENT_1>(&counter2);

    TEST_ASSERT_EQUAL(10, counter);
}

