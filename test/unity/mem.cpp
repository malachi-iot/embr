#include <estd/internal/platform.h>

#include "unit-test.h"

#if ESTD_OS_FREERTOS
#include <embr/platform/freertos/mem/pool.hpp>

#include <embr/mem/v1/functional.h>
#include <embr/mem/v1/functional/list.h>
#include <embr/mem/v1/shared-handle.h>

#include "../catch/mem/test-mem-data.h"

using namespace embr;

using pool_type = mem::freertos::layer1::pool<256, 4>;

#if FEATURE_EMBR_GLOBAL_GC
static void test_mem_gc_global()
{
    using namespace embr::mem::freertos;
    auto& pool = global_pool;
    int counter = 0;

    static_assert(sizeof(lock_handle) <= sizeof(int));
    static_assert(sizeof(shared_handle<int>) <= sizeof(int));
    static_assert(sizeof(lock_handle[4]) == 4);

    // Not implemented yet, but probably needed to rectify below DEBT
    pool.init();

    // DEBT: Watch out, global_pool initializes its mutex in an undefined way, which can
    // lead to crashes here
    int h = pool.alloc(15);

    {
        shared_handle<SideEffector> sh1 = make_shared<SideEffector>(&counter);
        TEST_ASSERT_EQUAL(1, counter);

        shared_handle<SideEffector> sh2 = make_shared<SideEffector>(&counter);
        TEST_ASSERT_EQUAL(2, counter);

        sh2()->counter_ = nullptr;
    }

    pool.gc();

    TEST_ASSERT_EQUAL(1, counter);

    {
        shared_handle<SideEffector> sh1 = make_shared<SideEffector>(&counter);
        TEST_ASSERT_EQUAL(2, counter);
    }

    TEST_ASSERT_EQUAL(1, counter);

    function<void(void)> f([&]{ ++counter; });

    f();

    TEST_ASSERT_EQUAL(2, counter);

    pool.gc();

    pool.dealloc(h);

    pool.gc();

    // TODO: Assert that we have full memory free again here
}

static void test_mem_gc_global_vector()
{
    using namespace embr::mem::freertos;

    vector<int> v1;

    v1.push_back(5);

    TEST_ASSERT_EQUAL(1, v1.size());
}

static void test_mem_gc_global_funclist()
{
    using namespace embr::mem::freertos;

    int counter = 0;

    funclist<void(int)> fl1;

    fl1 += [&](int v){ counter += v; };

    fl1.invoke(5);

    TEST_ASSERT_EQUAL(5, counter);
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
    RUN_TEST(test_mem_gc_global_funclist);
    RUN_TEST(test_mem_gc_global_vector);
#endif
    RUN_TEST(test_mem_gc_base);
    RUN_TEST(test_mem_gc_shared);
}

#endif
