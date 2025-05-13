#pragma once

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct BatteryHealthStatusBase
{
    enum flags_type : uint8_t
    {

    };
};

PACK(struct BatteryHealthStatus : BatteryHealthStatusBase   // NOLINT
{
    flags_type flags;
    uint8 summary;
    uint16 cycle_count;
    int8 temperature;
    uint16 deep_discharge_count;
});

template <>
struct characteristic_traits<BatteryHealthStatus>
{
    static constexpr const char* description()
    {
        return "Battery Health Status";
    }

    static constexpr v1::uuid::Characteristic16 uuid = v1::uuid::BatteryHealthStatus;
};

}}}}
