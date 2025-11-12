#include <functional>

#include <unity.h>

#include <estd/functional.h>

#include <embr/service/v2/enum.h>
#include <embr/platform/esp-idf/service/v2/service.h>

using namespace embr;

namespace embr::esp_idf::service::inline v2 {

// FIX: Horribly wrong.  Somehow linker can't pick up the one in embr/platform/esp-idf/service/v2/service.cpp
ESP_EVENT_DEFINE_BASE(SERVICE_EVENTS);

}

enum SYNTHETIC_EVENTS
{
    SYNTHETIC_EVENT_1,
    SYNTHETIC_EVENT_2
};

// Cleverly, this overrides visiblity of above enum.  However, that interferes
// with some of our template tricks.
// TODO: We might try some tricks of our own and
// put these defines in a different namespace
ESP_EVENT_DECLARE_BASE(SYNTHETIC_EVENTS);
ESP_EVENT_DEFINE_BASE(SYNTHETIC_EVENTS);

static constexpr const char* TEST1 = "SYNTHETIC_EVENTS";

TEST_CASE("state", "[service::state::v2]")
{
    using svc = embr::service::v2::service;
    using traits = esp_idf::service::v2::detail::state_base_traits<
        svc::substates, int>;

    esp_idf::service::v2::detail::state_base<traits> state;
}


struct service1 : esp_idf::service::v2::service
{
    using base_type = esp_idf::service::v2::service;
    using typename base_type::states;
    using typename base_type::substates;
    using base_type::state;

    void do_things(esp_event_loop_handle_t loop_handle)
    {
        state(substates::Starting, loop_handle);
    }

    void do_things()
    {
        state(substates::Starting);
    }
};


#define FEATURE_USER_EVENT_LOOP 0


TEST_CASE("service", "[service::v2]")
{
    // DEBT: Make our own event loop here, not the system one
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_event_loop_args_t loop_args
    {
        .queue_size = 5,
        .task_name = nullptr,
    };

    esp_event_loop_handle_t loop_handle;

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &loop_handle));

    namespace v2 = esp_idf::service::v2;

    service1 svc1;
    static bool started = false;
    int counter = 0;

    esp_idf::event::handler_register(
        v2::SERVICE_EVENTS, v2::SERVICE_CHANGING_STATE,
        [](void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
        {
            auto& e = *static_cast<service1::event_data*>(event_data);

            TEST_ASSERT_EQUAL_PTR(service1::property::state, e.name.data());
            TEST_ASSERT_EQUAL(esp_idf::service::v2::SERVICE_CHANGING_STATE, event_id);
            TEST_ASSERT_EQUAL(service1::Unstarted, e.changing_state);
            TEST_ASSERT_EQUAL(service1::Starting, e.changed_state);

            started = true;
        });

    auto f = [&](int32_t event_id, service1::event_data* event_data)
    {
        ++counter;
    };

    std::function f2([&](int32_t event_id, service1::event_data* event_data)
    {
        ++counter;
    });

#if FEATURE_USER_EVENT_LOOP
    ESP_ERROR_CHECK(svc1.handler_register(loop_handle, v2::SERVICE_CHANGING_STATE, f));
    ESP_ERROR_CHECK(svc1.handler_register(loop_handle, v2::SERVICE_CHANGING_STATE, f2));
#else
    ESP_ERROR_CHECK(svc1.handler_register(v2::SERVICE_CHANGING_STATE, f));
    ESP_ERROR_CHECK(svc1.handler_register(v2::SERVICE_CHANGING_STATE, f2));
#endif

#if FEATURE_USER_EVENT_LOOP
    svc1.do_things(loop_handle);

    ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 50));
    ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 50));
    ESP_ERROR_CHECK(esp_event_loop_run(loop_handle, 50));
#else
    svc1.do_things();
#endif

    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_EQUAL(2, counter);

    ESP_ERROR_CHECK(esp_event_loop_delete(loop_handle));
}