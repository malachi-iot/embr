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

// 05NOV26 MB TODO: Add:
// FreeRTOS local (non global, static) mutex flavor, to test that we really aren't
// copying estd::mutex around (which we are right now in embr::mem)

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

static void test_mem_gc_global_funclist_bigparam()
{
    using namespace embr::mem::freertos;

    struct thingy
    {
        int val1;
        const char* val2;
        double val3;
    };

    thingy v1{};

    funclist<void(thingy)> fl;

    fl += [&](thingy copy)
    {
        v1 = copy;
    };

    fl.invoke(thingy{0, "Hello", 5});
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


// Issue #59: gc/defrag must not move locked blocks
using pool8_type = mem::freertos::layer1::pool<256, 8>;

template <class Pool>
static void fill_pattern(Pool& pool, typename Pool::handle_type h, unsigned sz, uint8_t seed)
{
    auto d = (uint8_t*)pool.lock(h);
    for(unsigned i = 0; i < sz; ++i) d[i] = uint8_t(seed + i);
    pool.unlock(h);
}

template <class Pool>
static bool check_pattern(Pool& pool, typename Pool::handle_type h, unsigned sz, uint8_t seed)
{
    auto d = (const uint8_t*)pool.lock(h);
    bool ok = true;
    for(unsigned i = 0; i < sz; ++i) ok &= d[i] == uint8_t(seed + i);
    pool.unlock(h);
    return ok;
}

// Free | Locked | Free: the only defrag candidate is locked, so gc must leave it alone
static void test_mem_gc_locked()
{
    pool_type pool;

    int a = pool.alloc(32);
    int v = pool.alloc(16);
    int b = pool.alloc(32);

    fill_pattern(pool, v, 16, 0x40);
    auto data = (const uint8_t*)pool.lock(v);

    pool.dealloc(a);
    pool.dealloc(b);

    TEST_ASSERT_EQUAL(0, pool.gc());
    TEST_ASSERT_EQUAL_PTR(data, pool.ops().get_bundle(v).data());
    TEST_ASSERT_EQUAL_UINT8(0x40, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x4F, data[15]);

    pool.unlock(v);

    // Unlocked, it's a candidate again
    TEST_ASSERT_GREATER_THAN(0, pool.gc());
    TEST_ASSERT_NOT_EQUAL(data, pool.ops().get_bundle(v).data());
    TEST_ASSERT_TRUE(check_pattern(pool, v, 16, 0x40));
    TEST_ASSERT_TRUE(pool.ops().invariant().has_value());
}

// Nested lock: block stays put until lock count returns to zero
static void test_mem_gc_locked_nested()
{
    pool_type pool;

    int a = pool.alloc(32);
    int v = pool.alloc(16);
    int b = pool.alloc(32);
    int z = -1;

    // Consume the remainder so v is the only F-A-F candidate
    for(int sz = pool.ops().available().count(); z == -1 || z == 255; sz -= 4)
        z = pool.alloc(sz);

    fill_pattern(pool, v, 16, 0x60);
    const void* data = pool.lock(v);
    pool.lock(v);

    pool.dealloc(a);
    pool.dealloc(b);

    TEST_ASSERT_EQUAL(0, pool.gc());
    pool.unlock(v);
    TEST_ASSERT_EQUAL(0, pool.gc());
    TEST_ASSERT_EQUAL_PTR(data, pool.ops().get_bundle(v).data());
    pool.unlock(v);
    TEST_ASSERT_GREATER_THAN(0, pool.gc());
    TEST_ASSERT_NOT_EQUAL(data, pool.ops().get_bundle(v).data());
    TEST_ASSERT_TRUE(check_pattern(pool, v, 16, 0x60));
    TEST_ASSERT_TRUE(pool.ops().invariant().has_value());

    pool.dealloc(z);
}

// Locked and unlocked candidates side by side: gc compacts around the locked one
static void test_mem_gc_locked_neighbor()
{
    pool8_type pool;

    int f0 = pool.alloc(24);
    int l = pool.alloc(16);
    int f1 = pool.alloc(24);
    int u = pool.alloc(16);
    int f2 = pool.alloc(24);

    fill_pattern(pool, l, 16, 0x10);
    fill_pattern(pool, u, 16, 0x20);

    const void* l_data = pool.lock(l);
    const void* u_data = pool.ops().get_bundle(u).data();

    pool.dealloc(f0);
    pool.dealloc(f1);
    pool.dealloc(f2);

    int moves = 0;
    while(pool.gc() > 0 && moves < 8) ++moves;

    TEST_ASSERT_GREATER_THAN(0, moves);
    TEST_ASSERT_EQUAL_PTR(l_data, pool.ops().get_bundle(l).data());
    TEST_ASSERT_NOT_EQUAL(u_data, pool.ops().get_bundle(u).data());
    TEST_ASSERT_TRUE(check_pattern(pool, u, 16, 0x20));

    pool.unlock(l);

    TEST_ASSERT_TRUE(check_pattern(pool, l, 16, 0x10));
    TEST_ASSERT_TRUE(pool.ops().invariant().has_value());
}

// gc from inside a handler: invoke holds vector + functor blocks locked
static void test_mem_gc_funclist_invoke()
{
    pool8_type pool;
    int calls = 0;
    int moved_locked = 0;

    mem::v1::funclist<void(int), pool8_type> fl(&pool);

    int pad0 = pool.alloc(16);
    fl += [&](int)
    {
        const void* before[8] {};

        for(unsigned i = 0; i < 8; ++i)
        {
            auto bn = pool.ops().get_bundle(i);
            if(!bn.page->is_null() && bn.allocated() && bn.block->lock_count())
                before[i] = bn.data();
        }

        while(pool.gc() > 0) {}

        for(unsigned i = 0; i < 8; ++i)
            if(before[i] && pool.ops().get_bundle(i).data() != before[i])
                ++moved_locked;

        ++calls;
    };
    int pad1 = pool.alloc(16);
    fl += [&, marker = uint32_t(0xC0DE1234)](int v) { if(marker == 0xC0DE1234) calls += v; };
    int pad2 = pool.alloc(16);

    pool.dealloc(pad0);
    pool.dealloc(pad1);
    pool.dealloc(pad2);

    fl.invoke(10);

    TEST_ASSERT_EQUAL(11, calls);
    TEST_ASSERT_EQUAL(0, moved_locked);
    TEST_ASSERT_TRUE(pool.ops().invariant().has_value());

    calls = 0;
    fl.invoke(10);
    TEST_ASSERT_EQUAL(11, calls);
}


// gc moving a funclist functor must preserve its ref_count (FEATURE_EMBR_GC_FUNCLIST_REFCOUNT)
static void test_mem_gc_funclist_move_refcount()
{
    pool8_type pool;
    int calls = 0;
    {
        mem::v1::funclist<void(int), pool8_type> fl(&pool);

        fl += [&](int) { ++calls; };        // anchor, so vector block precedes functor below
        int pad0 = pool.alloc(32);
        auto h = fl.push_back([&](int v) { calls += v; });
        int pad1 = pool.alloc(32);

        pool.dealloc(pad0);
        pool.dealloc(pad1);

        const void* before = pool.ops().get_bundle(h.handle()).data();
        TEST_ASSERT_GREATER_THAN(0, pool.gc());
        TEST_ASSERT_NOT_EQUAL(before, pool.ops().get_bundle(h.handle()).data());
        TEST_ASSERT_EQUAL(1, pool.ops().get_bundle(h.handle()).ref_count());

        fl.invoke(10);
        TEST_ASSERT_EQUAL(11, calls);
    }
    TEST_ASSERT_EQUAL(0, pool.allocated());
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
    RUN_TEST(test_mem_gc_global_funclist_bigparam);
    RUN_TEST(test_mem_gc_global_vector);
#endif
    RUN_TEST(test_mem_gc_base);
    RUN_TEST(test_mem_gc_shared);
    RUN_TEST(test_mem_gc_locked);
    RUN_TEST(test_mem_gc_locked_nested);
    RUN_TEST(test_mem_gc_locked_neighbor);
    RUN_TEST(test_mem_gc_funclist_invoke);
    RUN_TEST(test_mem_gc_funclist_move_refcount);
}

#endif
