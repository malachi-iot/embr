#pragma once

// VCS_v10.pdf Section 3.2

#include "../fwd.h"
#include "../int.h"
#include "enum.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct VolumeControlPointBase
{
    enum Opcodes : uint8_t
    {
        RELATIVE_VOLUME_DOWN,
        RELATIVE_VOLUME_UP,
        UNMUTE_RELATIVE_VOLUME_DOWN,
        UNMUTE_RELATIVE_VOLUME_UP,
        SET_ABSOLUTE_VOLUME,
        UNMUTE,
        MUTE
    };
};

PACK(struct VolumeControlPoint : VolumeControlPointBase
{
    Opcodes opcode;
    uint8 change_counter;
    uint8 volume_setting;   // only for absolute opcode
});
    
}

template <>
struct characteristic_traits<v1::VolumeControlPoint>
{
    static constexpr const char* description()
    {
        return "Volume Control Point";
    }

    static constexpr auto uuid = v1::uuid::Characteristic16::VolumeControlPoint;
};


}}}
