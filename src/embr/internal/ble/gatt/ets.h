#pragma once

// Elapsed Time field characteristic is defined in
// Gatt Supplemental Service Section 3.77

// See ETS_v1.0.pdf

#include "../../pack.h"

namespace embr::internal::ble::gatt {

PACK(struct ElapsedTime
{
    /*
    enum Flags : uint8_t
    {
        TIME_OF_DAY             = 0x00,
        TIME_TICK               = 0x01,

        TIME_LOCAL              = 0x00,
        TIME_UTC                = 0x02,

        TIME_RESOLUTION_1S      = 0x00,
        TIME_RESOLUTION_100MS   = 0x01,
        TIME_RESOLUTION_1MS     = 0x02,
        TIME_RESOLUTION_100U    = 0x03,

        TIME_OFFSET_UNUSED      = 0x00
    };

    Flags flags; */
    uint8_t flags;
    uint8_t value[6];
    uint8_t sync_source;
    uint8_t offset;
});

}
