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
    COLOR_CHANGED,
    DOB_CHANGED,
};

}

EMBR_ESP_EVENT_DECLARE_PROP_BASE_NS(test, PROPERTY_EVENTS, prop_provider);
EMBR_ESP_PROP_TRAITS(test::AGE_CHANGED, int);
EMBR_ESP_PROP_TRAITS(test::COLOR_CHANGED, std::string_view);
EMBR_ESP_PROP_TRAITS(test::DOB_CHANGED, std::string_view);

namespace test {

class prop_provider
{
    prop::v1::property<test::AGE_CHANGED> age_;

public:
    prop_provider() : age_(0) {}

    void age(int v, esp_event_loop_handle_t loop_handle)
    {
        age_.set(v, this, loop_handle);
    }
};


class prop_consumer
{
public:
    prop_provider* provider{};
    int age{};

    void attach(esp_event_loop_handle_t loop_handle);
};

void prop_consumer::attach(esp_event_loop_handle_t loop_handle)
{
    esp_idf::event::handler_register_exp<test::AGE_CHANGED>(loop_handle,
        [](prop_consumer* self, event::event_data<test::AGE_CHANGED>* e)
    {
        if(e->origin == self->provider)
        {
            self->age = e->changed_state;
        }
    }, this);
}

}

ESP_EVENT_DEFINE_BASE(test::PROPERTY_EVENTS);

using test1 = embr_esp_event_traits_exp<test::AGE_CHANGED>;

static_assert(std::is_same_v<test1::data_type, prop::property_event_data<int, test::prop_provider>>);
static_assert(std::is_same_v<test1::payload_type, int>);

TEST_CASE("event-property", "[property]")
{
    static int updated_age;
    static std::string_view updated_color;
    std::string_view updated_dob;
    test::prop_provider provider;
    test::prop_consumer consumer{&provider};

    TEST_ASSERT_EQUAL_STRING("int", test1::payload_name);

    constexpr esp_event_loop_args_t loop_args
    {
        .queue_size = 5,
    };

    esp_event_loop_handle_t loop_handle;

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_handle));

    consumer.attach(loop_handle);

    prop::v1::property<test::AGE_CHANGED> age(30);
    prop::v1::property<test::COLOR_CHANGED> color("red");
    prop::v1::property<test::DOB_CHANGED> dob("07/04/1776");

    esp_idf::event::handler_register_exp<test::AGE_CHANGED>(loop_handle,
        [](event::event_data<test::AGE_CHANGED>* e)
    {
        if(e->origin == nullptr)    updated_age = e->changed_state;
    });

    esp_idf::handler_register<test::COLOR_CHANGED>(loop_handle,
        [](void* arg, esp_event_base_t, int32_t, void* event_data)
    {
        auto prop_data = static_cast<event::event_data<test::COLOR_CHANGED>*>(event_data);
        updated_color = prop_data->changed_state;
    });

    esp_idf::event::handler_register_exp<test::DOB_CHANGED>(loop_handle,
        [](std::string_view* update, event::event_data<test::DOB_CHANGED>* e)
    {
        *update = e->changed_state;
    }, &updated_dob);

    age.set(31, nullptr, loop_handle);
    color.set("blue", nullptr, loop_handle);
    dob.set("01/01/2000", nullptr, loop_handle);
    provider.age(7, loop_handle);

    TEST_ASSERT_EQUAL(31, age);
    TEST_ASSERT_EQUAL(0, updated_age);

    ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 20));
    //ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 20));

    TEST_ASSERT_EQUAL(7, consumer.age);
    TEST_ASSERT_EQUAL(31, updated_age);
    TEST_ASSERT_EQUAL_STRING("blue", updated_color.data());
    TEST_ASSERT_EQUAL_STRING("01/01/2000", updated_dob.data());

    ESP_ERROR_CHECK(esp_event_loop_delete(loop_handle));
}