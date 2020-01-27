#include <stdio.h>
#include <string.h>
#include "unity.h"

#include <embr/streambuf.h>
#include <estd/internal/platform.h>

#include <esp-helper.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static void print_banner(const char* text);

// TODO: Move a bunch of this out into a support area, ala the old
// "user_main.c"


extern "C" void app_main()
{
    static const char *TAG = "app_main";

    init_flash();
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif

    // --------

    print_banner("Running all the registered tests");
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();

    print_banner("Starting interactive test menu");
    /* This function will not return, and will be busy waiting for UART input.
     * Make sure that task watchdog is disabled if you use this function.
     */
    unity_run_menu();
}

static void print_banner(const char* text)
{
    printf("\n#### %s #####\n\n", text);
}
