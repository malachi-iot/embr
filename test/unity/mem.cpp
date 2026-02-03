#include <estd/internal/platform.h>

#include "unit-test.h"

#if ESTD_OS_FREERTOS
#include <embr/platform/freertos/mem/pool.hpp>

#include <embr/mem/v1/shared-handle.h>

#include "../catch/mem/test-mem-data.h"

using namespace embr;

using pool_type = mem::freertos::layer1::pool<256, 4>;

#if FEATURE_EMBR_GLOBAL_GC
namespace embr { namespace mem { namespace freertos { inline namespace v1 {

// DEBT: Put this guy in a different .cpp
global_pool_type global_pool;

}}}}

static void test_mem_gc_global()
{
    using namespace embr::mem::freertos;
    auto& pool = global_pool;
    int counter = 0;

    pool.init();

    // DEBT: Watch out, global_pool initializes its mutex in an undefined way, which can
    // lead to crashes here
    int h = pool.alloc(15);

    {
        shared_handle<SideEffector> sh1 = make_shared<SideEffector>(&counter);
        TEST_ASSERT_EQUAL(1, counter);
    }

    pool.gc();

    pool.dealloc(h);

    pool.gc();
}
#endif


static void test_mem_gc_base()
{
    pool_type pool;

    pool.init();

    int h = pool.alloc(15);

    TEST_ASSERT_EQUAL(0, h);

    void* data = pool.lock(h);

    TEST_ASSERT_NOT_NULL(data);

    pool.gc();

    pool.unlock(h);

    pool.gc();

    pool.dealloc(h);

    // 0 == no candidates found, suggesting fully defragmented
    TEST_ASSERT_EQUAL(0, pool.gc());
}

static void test_mem_gc_shared()
{
    pool_type pool;
    int counter = 0;

    // FIX: Still needs Mutex support
    {
        mem::v1::shared_handle<SideEffector, pool_type> sh =
            mem::v1::make_shared<SideEffector>(pool, &counter);

        TEST_ASSERT_EQUAL(1, counter);
    }
}


#ifdef ESP_IDF_TESTING
TEST_CASE("gc memory allocator", "[gc]")
#else
void test_mem_gc()
#endif
{
#if FEATURE_EMBR_GLOBAL_GC
    RUN_TEST(test_mem_gc_global);
#endif
    RUN_TEST(test_mem_gc_base);
    RUN_TEST(test_mem_gc_shared);
}

#endif
