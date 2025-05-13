#pragma once

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct BatteryEnergyStatusBase
{
    enum flags_type : uint8_t
    {
        EXTERNAL_SOURCE_POWER           = 0x01,
        PRESENT_VOLTAGE                 = 0x02,
        AVAILABLE_ENERGY                = 0x04,
        AVAILABLE_BATTERY_CAPACITY      = 0x08,
        AVAILABLE_ENERGY_AT_LAST_CHARGE = 0x10
    };
};

PACK(struct BatteryEnergyStatus : BatteryEnergyStatusBase   // NOLINT
{
    flags_type flags;
    medfloat16 external_power_source;
    medfloat16 present_voltage;
    medfloat16 available_energy;
    medfloat16 available_battery_capacity;
    medfloat16 charge_rate;
    medfloat16 available_energy_at_last_charge;
});

template <>
struct characteristic_traits<BatteryEnergyStatus>
{
    static constexpr const char* description()
    {
        return "Battery Energy Status";
    }

    static constexpr v1::uuid::Characteristic16 uuid = v1::uuid::BatteryEnergyStatus;
};

}}}}
