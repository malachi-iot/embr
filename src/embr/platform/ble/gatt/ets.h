#pragma once

#include "fwd.h"
#include "char/elapsed-time.h"

namespace embr { namespace ble { namespace gatt { namespace internal {

struct CurrentElapsedTimeBase : ElapsedTime
{
    enum ClockStatus : uint8_t
    {
        CLOCK_STATUS_NEEDS_TO_BE_SET = 0x01
    };

    enum ClockCapabilities : uint8_t
    {
        CLOCK_APPLIES_DST = 0x01,
        CLOCK_MANAGES_TZ = 0x02
    };
};

}}}}

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

// ETS_v1.0 Section 3.1.1.

PACK(struct CurrentElapsedTime : internal::CurrentElapsedTimeBase    // NOLINT
{
    ClockStatus status;
    ClockCapabilities capabilities;
});

template <>
struct characteristic_traits<CurrentElapsedTime>
{
    static constexpr const char* description()
    {
        return "Current Elapsed Time";
    }

    static constexpr v1::uuid::Characteristic16 uuid = v1::uuid::CurrentElapsedTime;
};

struct ElapsedTimeService
{
    // DEBT: This ought to go into a service_traits
    static constexpr v1::uuid::Services16 uuid = gatt::v1::uuid::ElapsedTimeService;
};

}}}}

