#pragma once

#include "../../../internal/pack.h"
#include "../../../internal/word/v2/fwd.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

// "All fields in a characteristic or descriptor are little endian unless otherwise stated."
// GATT Specification Supplement 2.4
// It is implied BLE integers on the whole default to little endian, but I am playing it safe
template <size_t bits, v2::word_options o = v2::word_options::none>
using word = v2::word<bits, o | v2::word_options::packed | v2::word_options::little_endian | v2::word_options::implicit>;

}}}}

