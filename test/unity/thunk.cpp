#include "unit-test.h"

#include <embr/thunk.h>

// TODO: Only enable during FreeRTOS

#define MULTICORE 1

#if ESP_PLATFORM && MULTICORE
// NOTE: Be careful.  'post' operations are expected to be very fast,
// but exotic captures might slow things down.
struct esp_hw_mutex_non_isr
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    bool lock()
    {
        taskENTER_CRITICAL(&mux);
        return true;
    }

    void unlock()
    {
        taskEXIT_CRITICAL(&mux);
    }
};

using hw_mutex = esp_hw_mutex_non_isr;
#else
struct hw_mutex
{

};
#endif

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