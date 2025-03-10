#include <esp_log.h>

#include <embr/dsp/precalc.h>

#include "perf.h"

static const char* TAG = "embr::perf::dsp";

void test_dsp()
{
    embr::dsp::init_sin_table();

    for(float v = 0; v < 1; v += 0.001)
    {

    }

    ESP_LOGI(TAG, "");

    for(float v = 0; v < 1; v += 0.001)
    {

    }

    ESP_LOGI(TAG, "");
}