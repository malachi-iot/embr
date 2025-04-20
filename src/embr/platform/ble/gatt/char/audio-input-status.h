#pragma once

// AICS_v1.0.pdf - Section 3.4

#include "../int.h"
#include "fwd.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct AudioInputStatus
{
    enum Statuses : int8_t
    {
        INACTIVE,
        ACTIVE
    };
};

}}}}
