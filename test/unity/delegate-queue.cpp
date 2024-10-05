#include "unit-test.h"

// So far only esp-idf is supported by way of xRingBuffer, but other
// queue mechanisms (especially "bip buffer") in theory could be employed

#if defined(ESP_PLATFORM)
#include <embr/internal/delegate_queue.h>

#include <esp_log.h>

typedef embr::internal::delegate_queue<> queue_type;

static constexpr const char* TAG = "delegate-queue";

static void test_delegate_queue1()
{
    queue_type q(512);
    int val = 0;

    ESP_LOGI(TAG, "max item size=%u, current free size=%u",
        q.buffer.max_item_size(),
        xRingbufferGetCurFreeSize(q.buffer));

    q.enqueue([&]{ ++val; }, portMAX_DELAY);
    q.enqueue([&]{ ++val; }, portMAX_DELAY);

    ESP_LOGI(TAG, "max item size=%u, current free size=%u",
        q.buffer.max_item_size(),
        xRingbufferGetCurFreeSize(q.buffer));

    q.dequeue(portMAX_DELAY);

    TEST_ASSERT_EQUAL(1, val);

    q.dequeue(portMAX_DELAY);

    TEST_ASSERT_EQUAL(2, val);
}

#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("delegate queue", "[delegate-queue]")
{
    RUN_TEST(test_delegate_queue1);
}
#else
void test_delegate_queue()
{
    // DEBT: I thought we redid this using a our own pseudo ring buffer? Guess not
}
#endif
