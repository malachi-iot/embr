#pragma once

// AICS_v1.0.pdf - Section 3.2

#include "../int.h"
#include "fwd.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

PACK(struct GainSettingProperties    // NOLINT
{
    uint8 units;
    int8 minimum;
    int8 maximum;
});

}}}}
