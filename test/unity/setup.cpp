#include "unit-test.h"

#if ESP_PLATFORM
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#endif

extern "C" void setUp (void)
{
#if ESP_PLATFORM
    // These two are in support of tcp_pcb testing - but so far it's not online.
    // Not needed for anything else
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    // Reports invalid state - is this already running under esp-idf unity?
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
#endif
}

extern "C" void tearDown (void)
{

}
