#pragma once

// VCS_v10.pdf Section 3.1

#include "../fwd.h"
#include "../int.h"
#include "enum.h"

namespace embr { namespace ble { namespace gatt {

inline namespace v1 {

PACK(struct VolumeState
{
    uint8 volume_setting;
    uint8 mute;
    uint8 change_counter;
});
    
}

template <>
struct characteristic_traits<v1::VolumeState>
{
    static constexpr const char* description()
    {
        return "Volume State";
    }

    static constexpr auto uuid = v1::uuid::Characteristic16::VolumeState;
};


}}}
