#pragma once

// Elapsed Time field characteristic is defined in
// Gatt Supplemental Service Section 3.77

// See ETS_v1.0.pdf

#include "../../pack.h"

namespace embr::internal::ble::gatt {

struct ElapsedTimeBase
{
    enum Flags : uint8_t
    {
        TIME_OF_DAY        = 0x00,
        TICK               = 0x01,

        LOCAL              = 0x00,
        UTC                = 0x02,

        RESOLUTION_1S      = 0x00,
        RESOLUTION_100MS   = 0x01,
        RESOLUTION_1MS     = 0x02,
        RESOLUTION_100U    = 0x03,

        OFFSET_UNUSED      = 0x00
    };

    //static constexpr uint8_t TICK   = 0x01, RESOLUTION_1S = 0;
};

PACK(struct ElapsedTime : ElapsedTimeBase
{
    uint8_t flags;
    uint8_t value[6];
    uint8_t sync_source;
    uint8_t offset;
});

}
