#pragma once

#include "esp_log.h"

#define EMBR_LOG_GROUP_MODE_DISABLED    0
#define EMBR_LOG_GROUP_MODE_ENABLED     1
#define EMBR_LOG_GROUP_MODE_ISR         2

#define EMBR_LOG_GROUP_0    CONFIG_EMBR_LOG_GROUP_0
#define EMBR_LOG_GROUP_1    CONFIG_EMBR_LOG_GROUP_1
#define EMBR_LOG_GROUP_2    CONFIG_EMBR_LOG_GROUP_2
#define EMBR_LOG_GROUP_3    CONFIG_EMBR_LOG_GROUP_3
#define EMBR_LOG_GROUP_4    CONFIG_EMBR_LOG_GROUP_4

#define ASSERT_EMBR_LOG_GROUP_MODE(group, mode)     \
static_assert(EMBR_LOG_GROUP_ ## group == mode || EMBR_LOG_GROUP_ ## group == 0,    \
    "embr log group " #group " must be mode " #mode);

#ifndef EMBR_LOG_GROUP_0
#define EMBR_LOG_GROUP_0 0
#endif

#ifndef EMBR_LOG_GROUP_1
#define EMBR_LOG_GROUP_1 0
#endif

#ifndef EMBR_LOG_GROUP_2
#define EMBR_LOG_GROUP_2 0
#endif

#ifndef EMBR_LOG_GROUP_3
#define EMBR_LOG_GROUP_3 0
#endif

#ifndef EMBR_LOG_GROUP_4
#define EMBR_LOG_GROUP_4 0
#endif

#ifndef EMBR_LOG_GROUP_5
#define EMBR_LOG_GROUP_5 0
#endif

// Copying do/while technique from esp_log.h - I presume it has to do with compiler optimizations

#if defined(__cplusplus) && (__cplusplus >  201703L)
#define ESP_GROUP_LOGI( group, tag, format, ... )   do {\
    if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ISR) ESP_DRAM_LOGI(tag, format __VA_OPT__(,) __VA_ARGS__); \
    else if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ENABLED) ESP_LOGI(tag, format __VA_OPT__(,) __VA_ARGS__); \
} while(0)
#else
#define ESP_GROUP_LOGI( group, tag, format, ... )   do {\
    if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ISR) ESP_DRAM_LOGI(tag, format, ##__VA_ARGS__); \
    else if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ENABLED) ESP_LOGI(tag, format, ##__VA_ARGS__); \
} while(0)
#endif

#define ESP_GROUP_LOGD( group, tag, format, ... )   do {\
    if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ISR) ESP_DRAM_LOGD(tag, format, ##__VA_ARGS__); \
    else if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ENABLED) ESP_LOGD(tag, format, ##__VA_ARGS__); \
} while(0)

#define ESP_GROUP_LOGV( group, tag, format, ... )   do {\
    if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ISR) ESP_DRAM_LOGV(tag, format, ##__VA_ARGS__); \
    else if(EMBR_LOG_GROUP_ ## group == EMBR_LOG_GROUP_MODE_ENABLED) ESP_LOGV(tag, format, ##__VA_ARGS__); \
} while(0)
