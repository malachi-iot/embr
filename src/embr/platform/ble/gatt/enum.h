#pragma once

#include <estd/cstdint.h>

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

namespace service { namespace uuid {

enum Services16 : uint16_t
{
    AudioInputControl = 0x1843,
    ObjectTransfer = 0x1825,
};

}}

}}}}
