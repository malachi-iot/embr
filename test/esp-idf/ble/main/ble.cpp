#include <esp_log.h>

#include <embr/platform/ble/gap/beacon.h>
#include <embr/platform/ble/gatt/aics.h>
#include <embr/platform/ble/gatt/bas.h>
#include <embr/platform/ble/gatt/ets.h>
#include <embr/platform/ble/gatt/ots.h>
#include <embr/platform/ble/gatt/vcs.h>
#include <embr/platform/nimble/gatt/session.h>
#include <embr/platform/nimble/uuid.h>

static const char* TAG = "embr::ble::test";

using namespace embr;
using namespace embr::ble::gatt::v1;

extern "C" void app_main(void)
{
    [[maybe_unused]]
    constexpr const ble_uuid_t* uuid1 = nimble::uuid<uuid::ObjectTransfer>;
    [[maybe_unused]]
    constexpr const ble_uuid_t* uuid2 = nimble::uuid<uuid::ObjectName>;

    ESP_LOGI(TAG, "app_main: entry");
}
