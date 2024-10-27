#pragma once

#include <estd/chrono.h>
#include <estd/cstdint.h>

#include "fwd.h"
#include "int.h"

// Elapsed Time field characteristic is defined in
// Gatt Supplemental Service Section 3.77

// See ETS_v1.0.pdf
// See GATT_Specification_Supplement.pdf (2024-02-21) 3.77 p.90

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

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

    // NOTE: PACK trickery prohibits these from residing inside 'ElapsedTime'
    using rep_1s = estd::chrono::duration<uint48>;
    using rep_100ms = estd::chrono::duration<uint48, estd::ratio<100, 1000>>;
    using rep_1ms = estd::chrono::duration<uint48, estd::milli>;
    using rep_100us = estd::chrono::duration<uint48, estd::ratio<100, 1000000>>;
};

PACK(struct ElapsedTime : ElapsedTimeBase
{
    uint8_t flags;
    uint48 value;
    //uint8_t value[6];
    uint8_t sync_source;
    uint8_t offset;

    rep_100ms as_100ms() const { return rep_100ms{ value }; }
    rep_1ms as_1ms() const { return rep_1ms{ value }; }
});

}}}}    // embr::ble::gatt::v1
