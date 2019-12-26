#include <experimental/observer-event-handler.hpp>
#include <embr/platform/freertos/exp/transport-retry.h>

//#include <estd/iostream.h>    // FIX: This fails rather badly, look into why
#include <embr/observer.h>

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
#else
    system_event_cb_t test =
#endif
        embr::experimental::esp_idf::event_handler<subject_type, Context>;

#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    ip_event_sta_got_ip_t dummy_data
    test(context, IP_EVENT, IP_EVENT_STA_GOT_IP, )
#else
    system_event_t dummy_data;
    dummy_data.event_id = SYSTEM_EVENT_STA_GOT_IP;
    test(&context, &dummy_data);
#endif

    TEST_ASSERT_EQUAL(1, context.got_ip_counter);
}


TEST_CASE("freertos retry", "[experimental]")
{
    
}