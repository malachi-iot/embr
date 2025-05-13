#pragma once

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct BatteryHealthInformationBase
{
    enum flags_type : uint8_t
    {

    };
};

PACK(struct BatteryHealthInformation : BatteryHealthInformationBase   // NOLINT
{
    flags_type flags;
});

template <>
struct characteristic_traits<BatteryHealthInformation>
{
    static constexpr const char* description()
    {
        return "Battery Health Information";
    }

    static constexpr v1::uuid::Characteristic16 uuid = v1::uuid::BatteryHealthInformation;
};

}}}}
