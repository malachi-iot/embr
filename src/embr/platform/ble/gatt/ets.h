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
    using rep = gatt::v1::word<48, v2::word_options::implicit>;

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
    using rep_1s = estd::chrono::duration<rep>;
    using rep_100ms = estd::chrono::duration<rep, estd::ratio<100, 1000>>;
    using rep_1ms = estd::chrono::duration<rep, estd::milli>;
    using rep_100us = estd::chrono::duration<rep, estd::ratio<100, 1000000>>;
};

PACK(struct ElapsedTime : ElapsedTimeBase
{
    uint8_t flags;
    uint48 value;
    uint8_t sync_source;
    uint8_t offset;

    const rep_1s& as_1s() const { return value.as<rep_1s>(); }
    const rep_100ms& as_100ms() const { return value.as<rep_100ms>(); }
    const rep_1ms& as_1ms() const { return value.as<rep_1ms>(); }
    const rep_100us& as_100us() const { return value.as<rep_100us>(); }
});

}}}}    // embr::ble::gatt::v1
