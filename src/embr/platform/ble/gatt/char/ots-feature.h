#pragma once

// OTS_v10.pdf [1.2] Section 3.1

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct OtsFeatureBase
{
    enum oacp_supported
    {
        CREATE          = 0 << 1,
        DELETE          = 1 << 1,
        CHECKSUM        = 2 << 1,
        EXECUTE         = 3 << 1,
        READ            = 4 << 1,
        WRITE           = 5 << 1,
        APPEND          = 6 << 1,
        TRUNCATE        = 7 << 1,
        PATCH           = 8 << 1,
        ABORT           = 9 << 1
    };
};

PACK(struct OtsFeature : OtsFeatureBase
{
    uint32 oacp;
    uint32 olcp;
});

template <>
struct characteristic_traits<OtsFeature>
{
    static constexpr const char* description()
    {
        return "OTS Feature";
    }

    static constexpr uint16_t uuid = 0x2ABD;
};


}}}}
