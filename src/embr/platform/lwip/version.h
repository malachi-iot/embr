#pragma once

#include <lwip/init.h>

#define EMBR_LWIP_VERSION(MAJOR, MINOR, REVISION, RC)   \
    ((MAJOR) << 24 | (MINOR) << 16 | (REVISION) << 8 | (RC))
