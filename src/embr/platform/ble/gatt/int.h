#pragma once

#include "../../../internal/word/v2/word.h"
#include "fwd.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

using int8 = int8_t;
using uint8 = uint8_t;
using uint16 = word<16>;
using uint24 = word<24>;
using uint32 = word<32>;
using uint48 = word<48>;

// DEBT: Firstly, not an integer.  Secondly,
// waiting on https://github.com/malachi-iot/embr/issues/20
// See GATT_Specification_Supplement.pdf section 2.1.1.
// These correspond to fixed point FLOAT and SFLOAT described
// here: https://build.fhir.org/ig/HL7/phd/MderFLOATsandSFLOATs.html
using medfloat32 = uint32;
using medfloat16 = uint16;


}}}}
