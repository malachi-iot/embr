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

using test1 = embr_esp_event_traits_exp<test::AGE_CHANGED>;

static_assert(std::is_same_v<test1::payload_type, prop::property_event_data<int, test::prop_provider>>);

TEST_CASE("event-property", "[property]")
{
    TEST_ASSERT_EQUAL_STRING("int", test1::payload_name);
}