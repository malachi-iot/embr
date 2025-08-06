#include <vector>
#include <set>

#include <esp_log.h>

#include "unity.h"

#include <embr/platform/esp-idf/allocator.h>

static const char* TAG = "unity::memory";

using namespace embr;

template <class T>
#if CONFIG_SPIRAM
using allocator = esp_idf::allocator<T, MALLOC_CAP_SPIRAM>;
#else
#if CONFIG_SOC_SPIRAM_SUPPORTED
#warning SPIRAM supported, but not enabled.  Recommended you enable it
#endif
using allocator = esp_idf::allocator<T, MALLOC_CAP_INTERNAL>;
#endif

TEST_CASE("caps allocator", "[allocator]")
{
    int* const litmus = allocator<int>::allocate(1);

    if(litmus == nullptr)
    {
        ESP_LOGI(TAG, "Can't allocate from SPIRAM, not doing caps allocator test");
        return;
    }

    allocator<int>::deallocate(litmus, 1);

    std::vector<int, allocator<int> > v;
    std::set<int, std::less<>, allocator<int> > s;

    v.push_back(5);

    TEST_ASSERT(true);
}