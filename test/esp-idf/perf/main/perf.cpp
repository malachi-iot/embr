#include <esp_log.h>

#include <embr/profiler.h>
#include <embr/dsp/precalc.h>

#include "perf.h"

static const char* TAG = "embr::perf";

extern "C" void app_main(void)
{
    test_dsp();
}
