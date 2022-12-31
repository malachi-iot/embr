// Requires 'bootloader_support' component

// https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-reference/system/app_image_format.html
#include <esp_app_format.h>

namespace embr { namespace esp_idf {

// What ESP32 variant specifically we are compiling for
constexpr esp_chip_id_t chip_id()
{
#if CONFIG_IDF_TARGET_ESP32
    return ESP_CHIP_ID_ESP32;
#elif CONFIG_IDF_TARGET_ESP32S2
    return ESP_CHIP_ID_ESP32S2;
#elif CONFIG_IDF_TARGET_ESP32S3
    return ESP_CHIP_ID_ESP32S3;
#elif CONFIG_IDF_TARGET_ESP32C3
    return ESP_CHIP_ID_ESP32C3;
#elif CONFIG_IDF_TARGET_ESP32H2
    return ESP_CHIP_ID_ESP32H2;
#elif CONFIG_IDF_TARGET_ESP32C2
    return ESP_CHIP_ID_ESP32C2;
#else
#warning Unidentified chipd id, falling back to generic ESP32
    return ESP_CHIP_ID_ESP32;
#endif
}


template <esp_chip_id_t>
struct chip_traits;


template <>
struct chip_traits<ESP_CHIP_ID_ESP32>
{
    static const char* name() { return "ESP32"; }
};



}}