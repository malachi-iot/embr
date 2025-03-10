#include <esp_log.h>

#include <estd/span.h>

#include "perf.h"

static const char* TAG = "embr::perf::storage";

// DEBT: This perf test actually belongs in estd

int raw_ptr(int* vals, int sz)
{
    int j = 0;

    for(int i = sz; i > 0; i--, vals++)
        j += *vals;

    return j;
}


int spanned(estd::span<int> vals)
{
    int j = 0;
    int* data = vals.data();

    for(int i = vals.size(); i > 0; i--, data++)
        j += *data;

    return j;
}

static int data[4096];

void test_span()
{
    int j = 0;
    for(int i = 0; i < 4096; i++)
        data[i] = i * 2;

    Profiler p;
    for(int i = 0; i < 1000; i++)
        j += raw_ptr(data, 4096);
    duration m1 = p.mark();

    ESP_LOGI(TAG, "raw j=%d %" PRIu64 "us", j, m1.count());

    p.reset();

    j = 0;
    for(int i = 0; i < 1000; i++)
        j += spanned(estd::span<int>(data, 4096));
    duration m2 = p.mark();

    ESP_LOGI(TAG, "span j=%d %" PRIu64 "us", j, m2.count());
}
