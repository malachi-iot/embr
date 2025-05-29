#pragma once

#include <estd/cstdint.h>

extern "C" {
#include <nimble/ble.h>
#include <host/ble_gap.h>
}

namespace embr { namespace nimble { namespace gap { inline namespace v1 {

struct Session
{
    uint8_t addr_type;
    uint16_t conn_handle;

    // See nimble/ble.h ble_error_codes enum
    int gap_terminate(uint8_t hci_reason = BLE_ERR_SUCCESS) const
    {
        return ble_gap_terminate(conn_handle, hci_reason);
    }

    int gap_conn_find(ble_gap_conn_desc* desc) const
    {
        return ble_gap_conn_find(conn_handle, desc);
    }

    int gap_update_params(const ble_gap_upd_params* params)
    {
        return ble_gap_update_params(conn_handle, params);
    }

    int gap_adv_start(const ble_addr_t* direct_addr, int32_t duration_ms,
        const ble_gap_adv_params* adv_params, ble_gap_event_fn* cb, void* cb_arg = nullptr)
    {
        return ble_gap_adv_start(addr_type, direct_addr, duration_ms, adv_params, cb, cb_arg);
    }

    int gap_adv_start(const ble_gap_adv_params* adv_params, ble_gap_event_fn* cb, void* cb_arg = nullptr)
    {
        return ble_gap_adv_start(addr_type, nullptr, BLE_HS_FOREVER, adv_params, cb, cb_arg);
    }

    int gap_security_initiate() const
    {
        return ble_gap_security_initiate(conn_handle);
    }
};

}}}}

