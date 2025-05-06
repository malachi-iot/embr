#include <unity.h>

#include <esp_check.h>
#include <esp_log.h>

#include <embr/platform/esp-idf/nvs.h>
#include <embr/esp-idf/nvs/cache.h>
#include <embr/esp-idf/nvs/streambuf.h>

using namespace embr;

static const char* TAG = "unity::nvs";

//char sector_buffer[4096];

static void test_nvs_ostreambuf()
{
    esp_idf::nvs::impl::ostreambuf<> os;
}


static void test_nvs_allocator()
{
    using type = esp_idf::nvs_allocator_base;

    [[maybe_unused]]    esp_err_t ret;
    type na;

    ESP_ERROR_CHECK(na.open());
    if(na.is_formatted() == false)
    {
        ESP_LOGI(TAG, "Formatting embr partition");
        ESP_GOTO_ON_ERROR(na.format(), err, TAG, "Format failed");
    }

    {
        TEST_ASSERT_EQUAL(ESP_OK, na.mmap(0));

        const type::header* header = na.data();

        TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, na.mmap(0, estd::chrono::milliseconds(50)));

        TEST_ASSERT_EQUAL(1, header->size_in_blocks);
        TEST_ASSERT_EQUAL(0, header->id);

        na.munmap();
        na.close();
    }

    return;

err:
    na.close();
    TEST_FAIL_MESSAGE("Aborted");
}

TEST_CASE("nvs wrappers", "[nvs]")
{
    esp_idf::nvs::Handle h;

    h.open("embr::unity", NVS_READONLY);
    h.close();

    RUN_TEST(test_nvs_allocator);
    RUN_TEST(test_nvs_ostreambuf);
}