#include <unity.h>

#include <esp_log.h>

#include <embr/platform/esp-idf/nvs.h>
#include <embr/esp-idf/nvs-allocator.h>

using namespace embr;

static void test_nvs_allocator()
{
    esp_idf::nvs_allocator_base na;

    ESP_ERROR_CHECK(na.open());
    na.mmap(0);

    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, na.mmap(0, estd::chrono::milliseconds(50)));
    na.munmap();
}

TEST_CASE("nvs wrappers", "[nvs]")
{
    esp_idf::nvs::Handle h;

    h.open("embr::unity", NVS_READONLY);
    h.close();

    RUN_TEST(test_nvs_allocator);
}