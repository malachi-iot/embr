#include <unity.h>

#include <embr/service/v2/enum.h>
#include <embr/platform/esp-idf/service/v2/service.h>

using namespace embr;

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


/*
struct service1 : esp_idf::service::v2::service
{
    using base_type = esp_idf::service::v2::service;
    using typename base_type::states;
    using typename base_type::substates;
    using base_type::state;
};*/


TEST_CASE("service", "[service::v2]")
{
}