// DEBT: Simplistic wrapper to bring things online
#pragma once

#include <stdio.h>

// DEBT: this approach REQUIRES at least one parameter
#define ESP_LOGW(tag, fmt, ...)     { printf("%s: ", tag); printf(fmt, __VA_ARGS__); puts(""); }
#define ESP_LOGI(tag, fmt, ...)     { printf("%s: ", tag); printf(fmt, __VA_ARGS__); puts(""); }
#define ESP_LOGD(tag, fmt, ...)     { printf("%s: ", tag); printf(fmt, __VA_ARGS__); puts(""); }
#define ESP_LOGV(tag, fmt, ...)     { printf("%s: ", tag); printf(fmt, __VA_ARGS__); puts(""); }

