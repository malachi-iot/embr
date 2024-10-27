#pragma once

#include <estd/cstdint.h>

#include "fwd.h"

// 26OCT24 DEBT: Pretty sure this is a GAP thing, but advertise might not be correctly in that
// category.  Once I'm sure replace this comment with the facts (and move code if necessary)

namespace embr::ble::gap { inline namespace v1 {

PACK(struct Beacon
{
    uint16_t type;
    uint8_t uuid[16];
    uint16_t major;
    uint16_t minor;
    int8_t measured_power;
});


}}
