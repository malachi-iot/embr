#pragma once

#include <estd/cstdint.h>

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

namespace uuid {

enum Services16 : uint16_t
{
    AudioInputControl = 0x1843,
    BatteryService = 0x180F,
    CommonAudioService = 0x1853,
    ElapsedTimeService = 0x183F,
    ObjectTransfer = 0x1825,
    VolumeControlService = 0x1844
};

}

namespace service { namespace uuid {

// OBSOLETE, use gatt::v1::uuid flavor instead
using Services16 = gatt::v1::uuid::Services16;

}}

}}}}
