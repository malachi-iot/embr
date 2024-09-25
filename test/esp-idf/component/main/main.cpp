#include <esp_log.h>

#include <estd/port/version.h>

static const char* TAG = "embr::test::component";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "app_main: estd version %s", ESTD_VERSION_STR);
}
