#include "unit-test.h"

#include <embr/thunk.h>

#if ESTD_OS_FREERTOS

// NOTE: Be careful.  'post' operations are expected to be very fast,
// but exotic captures might slow things down.
using hw_mutex = embr::internal::freertos_hw_mutex;

static void test_thunk_ll()
{
    int counter = 0;
    embr::sys::detail::v1::thunk<estd::layer1::bipbuf<256>, hw_mutex> thunk;

    thunk.post([&] { ++counter; });
    thunk.poll_one();

    TEST_ASSERT_EQUAL(1, counter);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("thunk", "[thunk]")
#else
void test_thunk()
#endif
{
    RUN_TEST(test_thunk_ll);
}

#endif
