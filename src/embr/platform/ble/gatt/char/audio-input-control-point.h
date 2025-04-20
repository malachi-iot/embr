#pragma once

// AICS_v1.0.pdf - Section 3.5

#include "../int.h"
#include "fwd.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct AudioInputControlPointBase
{
    enum Opcodes : int8_t
    {
        SET_GAIN_SETTING,
        UNMUTE,
        MUTE,
        SET_MANUAL_GAIN_MODE,
        SET_AUTOMATIC_GAIN_MODE
    };
};

PACK(struct AudioInputControlPoint : AudioInputControlPointBase   // NOLINT
{
    Opcodes opcode;
    uint8_t change_counter;
    int8_t gain_setting;
});

}}}}
