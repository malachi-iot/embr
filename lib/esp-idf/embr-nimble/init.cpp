#include <esp_event.h>

#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>

#include <nimble/ble.h>
#include <host/ble_gap.h>

extern "C" {
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <services/dis/ble_svc_dis.h>
#include <services/cts/ble_svc_cts.h>
}

#include "embr/nimble/event.h"
#include "embr/nimble/fwd.h"

static const char* TAG = "embr::nimble";

ESP_EVENT_DEFINE_BASE(EMBR_NIMBLE_GAP_EVENT);
ESP_EVENT_DEFINE_BASE(EMBR_NIMBLE_GATT_EVENT);


static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    ESP_ERROR_CHECK(esp_event_post(EMBR_NIMBLE_GAP_EVENT, event->type, NULL, 0, portMAX_DELAY));
    return 0;
}


static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "Host Task Started");

    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

namespace embr::nimble {

inline namespace gap {

enum events
{
    CONNECT = BLE_GAP_EVENT_CONNECT,
    DISCONNECT = BLE_GAP_EVENT_DISCONNECT,
};

}

//using EMBR_NIMBLE_GAP_EVENT = embr::nimble::gap::events;

void init_embr_nimble()
{
    ESP_ERROR_CHECK(nimble_port_init());

    ble_svc_gap_init();
    ble_svc_gatt_init();

    nimble_port_freertos_init(ble_host_task);
}

}

