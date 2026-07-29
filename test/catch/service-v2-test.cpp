#include <catch2/catch_all.hpp>

#include <embr/service/v2/enum.h>
#include <estd/string_view.h>

using namespace embr;
using namespace embr::service;

TEST_CASE("Services v2", "[services-v2]")
{
    SECTION("basics")
    {
        using service = v2::service;

        service s{service::Running};

        REQUIRE(s.state() == service::Started);

        s.substate(service::ErrConfig);

        REQUIRE(s.state() == service::Error);

        s.substate(service::Sleeping);

        REQUIRE(s.state() == service::Stopped);

        REQUIRE(estd::string_view(to_string(s.state())) == "Stopped");
    }
}
