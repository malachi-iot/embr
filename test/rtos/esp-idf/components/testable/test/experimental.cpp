#include <experimental/observer-event-handler.hpp>
#include <embr/platform/freertos/exp/transport-retry.h>
#include <embr/platform/lwip/transport.h>

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


TEST_CASE("freertos retry", "[experimental]")
{
    using namespace embr::experimental;

    typedef embr::lwip::experimental::TransportUdp<> transport_type;
    RetryManager<transport_type, DummyReplyPolicy> rm;

    transport_type::ostreambuf_type out(128);
    transport_type::endpoint_type endpoint(nullptr, 0);

    rm.send(out, endpoint, 7);

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