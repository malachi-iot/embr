#include <estd/cstdint.h>

extern "C" {
#include <nimble/ble.h>
#include <host/ble_gap.h>
}

namespace embr { namespace nimble { namespace gap { inline namespace v1 {

struct Session
{
    uint8_t ble_addr_type;
    uint16_t conn_handle;

    // Unsure what formal values apply to "reason code", if any
    int gap_terminate(uint8_t hci_reason = 0) const
    {
        return ble_gap_terminate(conn_handle, 0);
    }

    int gap_conn_find(ble_gap_conn_desc* desc) const
    {
        return ble_gap_conn_find(conn_handle, desc);
    }
};

}}}}

