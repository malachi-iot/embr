#include <esp_log.h>

#include <unity.h>

#include <embr/platform/esp-idf/property/v1/property.h>

static const char* TAG = "embr::unity::property";

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

using namespace embr;
using namespace embr::esp_idf;

namespace test {

class prop_provider;

enum PROPERTY_EVENTS
{
    AGE_CHANGED,
    COLOR_CHANGED
};

}

EMBR_ESP_EVENT_DECLARE_PROP_BASE_NS(test, PROPERTY_EVENTS, prop_provider);
EMBR_ESP_PROP_TRAITS(test::AGE_CHANGED, int);
EMBR_ESP_PROP_TRAITS(test::COLOR_CHANGED, std::string_view);

namespace test {

class prop_provider
{
};

}

ESP_EVENT_DEFINE_BASE(test::PROPERTY_EVENTS);

using test1 = embr_esp_event_traits_exp<test::AGE_CHANGED>;

static_assert(std::is_same_v<test1::data_type, prop::property_event_data<int, test::prop_provider>>);
static_assert(std::is_same_v<test1::payload_type, int>);

TEST_CASE("event-property", "[property]")
{
    static int updated_age;
    static std::string_view updated_color;

    TEST_ASSERT_EQUAL_STRING("int", test1::payload_name);

    constexpr esp_event_loop_args_t loop_args
    {
        .queue_size = 5,
    };

    esp_event_loop_handle_t loop_handle;

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_handle));

    prop::v1::property<test::AGE_CHANGED> age(30);
    prop::v1::property<test::COLOR_CHANGED> color("red");

    esp_idf::event::handler_register_exp<test::AGE_CHANGED>(loop_handle,
        [](void*, prop::event_data<test::AGE_CHANGED>* e)
    {
        updated_age = e->changed_state;
    });

    esp_idf::handler_register_exp2<test::COLOR_CHANGED>(loop_handle,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
    {
        auto prop_data = static_cast<prop::event_data<test::COLOR_CHANGED>*>(event_data);
        updated_color = prop_data->changed_state;
    });

    age.set(31, nullptr, loop_handle);
    color.set("blue", nullptr, loop_handle);

    TEST_ASSERT_EQUAL(31, age);
    TEST_ASSERT_EQUAL(0, updated_age);

    ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 20));
    //ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 20));

    TEST_ASSERT_EQUAL(31, updated_age);
    TEST_ASSERT_EQUAL_STRING("blue", updated_color.data());

    ESP_ERROR_CHECK(esp_event_loop_delete(loop_handle));
}