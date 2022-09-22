#pragma once

#include <lwip/init.h>

// Adapted from LwIP's init.h itself:
#define EMBR_LWIP_VERSION(MAJOR, MINOR, REVISION, RC)   \
    ((MAJOR) << 24 | (MINOR) << 16 | (REVISION) << 8 | (RC))
