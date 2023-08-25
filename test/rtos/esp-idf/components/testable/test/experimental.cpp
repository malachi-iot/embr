#include <experimental/observer-event-handler.hpp>
#include <embr/platform/freertos/exp/transport-retry.h>
#include <embr/platform/lwip/transport.h>
#include <embr/exp/platform/freertos/fasio.h>

//#include <estd/iostream.h>    // FIX: This fails rather badly, look into why
#include <embr/observer.h>

// ESTD needs this to know to to include FreeRTOS .h ESP-style
#define ESP32

#include <estd/chrono.h>
// FIX: Not quite ready for prime time, test in estd area not here
//#include <estd/thread.h>

#include "unity.h"

#include "esp_log.h"

struct Empty {};

struct Context
{
    int got_ip_counter = 0;
};

struct Populator
{
    static void on_notify(embr::experimental::esp_idf::events::ip::got_ip e, Context& context)
    {
        context.got_ip_counter++;
    }
};

TEST_CASE("observer event handler", "[experimental]")
{
    using namespace embr::experimental::esp_idf;

    typedef embr::layer0::subject<Empty, Populator> subject_type;

    Context context;

#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    esp_event_handler_t test = 
        embr::experimental::esp_idf::ip_event_handler<subject_type, Context>;
#else
    system_event_cb_t test =
        embr::experimental::esp_idf::event_handler<subject_type, Context>;
#endif

#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    ip_event_got_ip_t dummy_data;
    // dummy assignment to hold off uninitialized variable warning/errors
    dummy_data.ip_info.ip = esp_ip4_addr();
    test(&context, IP_EVENT, IP_EVENT_STA_GOT_IP, &dummy_data);
#else
    system_event_t dummy_data;
    dummy_data.event_id = SYSTEM_EVENT_STA_GOT_IP;
    test(&context, &dummy_data);
#endif

    TEST_ASSERT_EQUAL(1, context.got_ip_counter);
}


static int dummy_item_count = 0;

struct DummyReplyPolicy
{
    typedef int key_type;
    typedef embr::lwip::experimental::TransportUdp<> transport_type;
    typedef transport_type::endpoint_type endpoint_type;
    typedef uint32_t timebase_type;

    static constexpr const char* TAG = "DummyReplyPolicy";

    struct item_policy_impl_type
    {
        static constexpr const char* TAG = "DummyReplyPolicy::item_policy_type";

        int count;

        SemaphoreHandle_t semaphore;

        item_policy_impl_type()
        {
            count = 0;
        }

        bool retry_done() const { return count != 0; }

        timebase_type get_new_expiry() const
        {
            return 100;
        }

        void process_timeout()
        { 
            ESP_LOGI(TAG, "process_timeout");
            count++;
            dummy_item_count++;
        }

        ~item_policy_impl_type()
        {
            ESP_LOGI(TAG, "dtor");
        }
    };

    bool match(const endpoint_type& incoming, const endpoint_type& outgoing)
    {
        // NOTE: this is a pointer compare, so be very careful here
        return incoming.address() == outgoing.address();
    }
};


// FIX: Dies with a stack overflow - disabling because this retry manager has been a pain
// in compilation for a while now and it was never completed anyway
#if ENABLE_EXP_RETRY_TEST
TEST_CASE("freertos retry", "[experimental]")
{
    using namespace embr::experimental;

    typedef embr::lwip::experimental::TransportUdp<> transport_type;
    RetryManager<transport_type, DummyReplyPolicy> rm;

    transport_type::ostreambuf_type out(128);
    transport_type::endpoint_type endpoint(nullptr, 0);

    rm.send(endpoint, out, 7);

    //estd::chrono::freertos_clock clock;
    //estd::this_thread::sleep_for(0.1s);
    int ms_left_to_wait = 1000;
    constexpr int ms_wait_period = 100;

    while(ms_left_to_wait > 0 && dummy_item_count == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(ms_wait_period));
        ms_left_to_wait -= ms_wait_period;
    }

    TEST_ASSERT_EQUAL(1, dummy_item_count);
}
#endif

static embr::experimental::fasio fasio;
static embr::experimental::fasio2 fasio2;


TEST_CASE("fake asio", "[experimental]")
{
    int counter = 0;

    auto f = fasio.make_inline([&]()
    {
        ++counter;
    });

    uint32_t handle = fasio.begin_invoke(f);
    handle = fasio.begin_invoke(f);

    fasio.service();
    fasio.service();

    fasio.end_invoke(handle);

    TEST_ASSERT_EQUAL(2, counter);

    BaseType_t r = fasio.end_invoke(handle, 10);

    TEST_ASSERT_EQUAL(pdFALSE, r);
}

static int test_fn(int value)
{
    return value * 2;
}


TEST_CASE("fake asio2", "[experimental]")
{
    int counter = 0;

    uint32_t handle;
    
    handle = fasio2.begin_invoke(
        [&](){ ++counter; });
    handle = fasio2.begin_invoke(
        [&](){ ++counter; });

    fasio2.service();
    fasio2.service();

    fasio2.end_invoke(handle);

    TEST_ASSERT_EQUAL(2, counter);

    //BaseType_t r = fasio2.end_invoke(handle, 10);

    //TEST_ASSERT_EQUAL(pdFALSE, r);

    auto v = fasio2.buffer.test2([](){ return 5; });
    auto v2 = fasio2.buffer.test2(test_fn, 10);

    TEST_ASSERT_EQUAL(20, v2.retval);

#if __cpp_generic_lambdas
    auto v3 = fasio2.buffer.test3(test_fn, 10);

    TEST_ASSERT_FALSE(v3.valid());

    fasio2.buffer.dequeue(portMAX_DELAY);

    TEST_ASSERT_TRUE(v3.valid());
    TEST_ASSERT_EQUAL(20, v3.get());
#endif
}