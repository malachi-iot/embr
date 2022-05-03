#include <esp-helper.h>

#include <esp_log.h>

void udp_echo_init();

extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_flash();

    ESP_LOGI(TAG, "Startup");
}

