#pragma once

// OTS_v10.pdf [1.2] Section 3.3

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

PACK(struct BatteryEnergyStatus
{
    uint8_t flags;
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
