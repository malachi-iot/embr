#include <unity.h>

#include <embr/service/v2/enum.h>
#include <embr/platform/esp-idf/service/v2/state.h>

using namespace embr;

TEST_CASE("service", "[service::v2]")
{
    using svc = embr::service::v2::service;
    using traits = esp_idf::service::v2::detail::state_base_traits<svc::substates, int>;

    esp_idf::service::v2::detail::state_base<traits> state;
}