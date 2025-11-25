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

enum SYNTHETIC_EVENTS3
{
    SYNTHETIC3_EVENT_1,
    SYNTHETIC3_EVENT_2
};

//using SYNTHETIC_EVENTS3_preserved = SYNTHETIC_EVENTS3;

// FIX: Trouble in TU paradise
EMBR_ESP_EVENT_DECLARE_BASE(SYNTHETIC_EVENTS2);

EMBR_ESP_EVENT_DECLARE_BASE_EXP(SYNTHETIC_EVENTS3);
ESP_EVENT_DEFINE_BASE(SYNTHETIC_EVENTS3);
//EMBR_ESP_EVENT_BASE_TRAITS(SYNTHETIC_EVENTS3);

//static constexpr esp_event_base_t TEST2 = SYNTHETIC_EVENTS2;

EMBR_ESP_EVENT_TRAITS(SYNTHETIC_EVENTS2, SYNTHETIC2_EVENT_1, int);

EMBR_ESP_EVENT_TRAITS_EXP(SYNTHETIC_EVENTS3, SYNTHETIC3_EVENT_1, int);
EMBR_ESP_EVENT_TRAITS_EXP2(SYNTHETIC3_EVENT_2, int);

TEST_CASE("event experimentation", "[event-exp]")
{
    constexpr esp_event_loop_args_t loop_args
    {
        .queue_size = 5,
    };

    esp_event_loop_handle_t loop_handle;

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_handle));

    using type = embr_esp_event_traits_exp<SYNTHETIC3_EVENT_2>;

    static_assert(std::is_same_v<type::type, SYNTHETIC_EVENTS3_preserved>);

    int v = 5;
    int counter = 0;

    auto f = [&](int* val)
    {
        counter += *val;
    };

    esp_idf::event::handler_register_exp<SYNTHETIC3_EVENT_2>(loop_handle, f);

    esp_idf::event::post_exp<SYNTHETIC3_EVENT_2>(loop_handle, &v);

    ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 20));

    TEST_ASSERT_EQUAL(5, counter);

    ESP_ERROR_CHECK(esp_event_loop_delete(loop_handle));
}

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

