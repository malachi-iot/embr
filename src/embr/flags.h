#pragma once

#include <estd/flags.h>

// Auto-promotes 'Enum' to flags<Enum> during these operations
// 07MAR25 MB DEPRECATED - call ESTD_FLAGS directly
#define EMBR_FLAGS(Enum)    ESTD_FLAGS(Enum)

