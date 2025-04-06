extern "C" {
#include <host/ble_gatt.h>
}

#include "../gap/session.h"

namespace embr { namespace nimble { namespace gatt { inline namespace v1 {

/* 05APR25 MB - pulled this in from missme.esp, but it has a code smell to it so leaving
   disabled
struct Session : gap::v1::Session
{
    int gattc_disc_all_svcs(ble_gatt_disc_svc_fn fn)
    {
        return ble_gattc_disc_all_svcs(conn_handle, fn, this);
    }

    int gattc_disc_all_chrs(uint16_t start_handle, uint16_t end_handle,
        ble_gatt_chr_fn fn)
    {
        return ble_gattc_disc_all_chrs(conn_handle, start_handle, end_handle,
            fn, this);
    }

    int gattc_write(uint16_t attr_handle, os_mbuf* om, ble_gatt_attr_fn* cb)
    {
        return ble_gattc_write(conn_handle, attr_handle, om, cb, this);
    }

    int gattc_exchange_mtu(ble_gatt_mtu_fn fn = nullptr, void* arg = nullptr)
    {
        return ble_gattc_exchange_mtu(conn_handle, fn, arg);
    }

    int gatts_indicate(uint16_t conn_handle, uint16_t chr_val_handle)
    {
        return ble_gatts_indicate(conn_handle, chr_val_handle);
    }
};
*/

}}}}

