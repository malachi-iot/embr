#include <estd/internal/platform.h>

#include "unit-test.h"

#if ESTD_OS_FREERTOS
#include <embr/platform/freertos/mem/pool.hpp>

using namespace embr::mem;

#ifdef ESP_IDF_TESTING
TEST_CASE("gc memory allocator", "[gc]")
#else
void test_mem_gc()
#endif
{
    freertos::layer1::pool<256, 4> pool;

    pool.init();

    int h = pool.alloc(15);

    TEST_ASSERT_EQUAL(0, h);

    void* data = pool.lock(h);

    TEST_ASSERT_NOT_NULL(data);

    pool.unlock(h);

    pool.dealloc(h);
}

#endif
