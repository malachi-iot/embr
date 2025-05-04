#include <unity.h>

#include <esp_log.h>

#include <embr/platform/esp-idf/nvs.h>
#include <embr/esp-idf/nvs-allocator.h>

using namespace embr;

TEST_CASE("nvs wrappers", "[nvs]")
{
    esp_idf::nvs::Handle h;

    h.open("embr::unity", NVS_READONLY);
    h.close();
}