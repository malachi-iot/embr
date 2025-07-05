#include <esp_log.h>

#include <embr/platform/ble/gap/beacon.h>
#include <embr/platform/ble/gatt/aics.h>
#include <embr/platform/ble/gatt/bas.h>
#include <embr/platform/ble/gatt/ets.h>
#include <embr/platform/ble/gatt/ots.h>
#include <embr/platform/ble/gatt/vcs.h>
#include <embr/platform/nimble/gatt/session.h>

static const char* TAG = "embr::ble::test";

using namespace embr;
using namespace embr::ble::gatt::v1;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "app_main: entry");
}
