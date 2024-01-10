// Used to require 'bootloader_support' component, but that's a little annoying just to get
// at one enum, so duplicating it via 'chip_ids' below

// https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-reference/system/app_image_format.html
//#include <esp_app_format.h>

namespace embr { namespace esp_idf {

enum class chip_id
{
    ESP32,
    ESP32S2,
    ESP32S3,
    ESP32C3,
    ESP32H2,
    ESP32C2,
    ESP32C6,

#if CONFIG_IDF_TARGET_ESP32
    Current = ESP32
#elif CONFIG_IDF_TARGET_ESP32S2
    Current = ESP32S2
#elif CONFIG_IDF_TARGET_ESP32S3
    Current = ESP32S3
#elif CONFIG_IDF_TARGET_ESP32C3
    Current = ESP32C3
#elif CONFIG_IDF_TARGET_ESP32H2
    Current = ESP32H2
#elif CONFIG_IDF_TARGET_ESP32C2
    Current = ESP32C2
#elif CONFIG_IDF_TARGET_ESP32C6
    Current = ESP32C6
#else
#warning Unidentified chipd id, falling back to generic ESP32
    Current = ESP32
#endif
};



template <chip_id = chip_id::Current>
struct chip_traits;


template <>
struct chip_traits<chip_id::ESP32>
{
    static constexpr const char* name() { return "ESP32"; }
};

template <>
struct chip_traits<chip_id::ESP32C3>
{
    static constexpr const char* name() { return "ESP32C3"; }
};

template <>
struct chip_traits<chip_id::ESP32C6>
{
    static constexpr const char* name() { return "ESP32C6"; }
};

template <>
struct chip_traits<chip_id::ESP32S3>
{
    static constexpr const char* name() { return "ESP32S3"; }
};


inline namespace internal {

template <chip_id = chip_id::Current>
struct pm_traits;

}

}}