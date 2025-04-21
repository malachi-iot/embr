#pragma once

#include <estd/cstdint.h>

namespace embr { namespace ble { namespace gatt {

inline namespace v1 { namespace uuid {

// Assigned_Numbers.pdf Section 3.8.1

enum Characteristic16 : uint16_t
{
    AudioInputControlPoint = 0x2B7B,
    AudioInputDescription = 0x2B7C,
    AudioInputState = 0x2B77,
    AudioInputStatus = 0x2B7A,
    AudioInputType = 0x2B79,
    GainSettingProperties = 0x2B78,
    ObjectName = 0x2ABE,
    ObjectSize = 0x2AC0,
    ObjectProperties = 0x2AC4,
    ObjectActionControlPoint = 0x2AC5,
    ObjectListControlPoint = 0x2AC6,
    ObjectChanged = 0x2AC8,
    ObjectId = 0x2AC3,
    VolumeControlPoint = 0x2B7E,
    VolumeFlags = 0x2B7F,
    VolumeState = 0x2B7D,
};

}}

}}}

