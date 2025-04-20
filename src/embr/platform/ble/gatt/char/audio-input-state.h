#pragma once

// AICS_v1.0.pdf - Section 2.2.1

#include "../int.h"
#include "fwd.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

PACK(struct AudioInputState    // NOLINT
{
    int8 gain_setting;
    uint8 mute;
    uint8 gain_mode;
    uint8 change_counter;
});

}}}}
